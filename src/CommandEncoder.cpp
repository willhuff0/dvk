#include <dvk/CommandEncoder.hpp>
#include <dvk/Config.hpp>
#include "dvk/Device.hpp"

#include <stdexcept>

using namespace dvk;

CommandEncoder::CommandEncoder(const Device& device, uint32_t framesInFlight)
    : device(device), framesInFlight(framesInFlight) {
    frameCommandPool = vk::raii::CommandPool(device.GetDevice(), {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = device.GetQueueFamilyIndex(),
    });
#if defined(DVK_ENABLE_DEBUG_LABELS)
    device.GetDevice().setDebugUtilsObjectNameEXT(*frameCommandPool, "DVK Frame");
#endif

    oneTimeSubmitCommandPool = vk::raii::CommandPool(device.GetDevice(), {
        .flags = vk::CommandPoolCreateFlagBits::eTransient,
        .queueFamilyIndex = device.GetQueueFamilyIndex(),
    });
#if defined(DVK_ENABLE_DEBUG_LABELS)
    device.GetDevice().setDebugUtilsObjectNameEXT(*oneTimeSubmitCommandPool, "DVK OneTimeSubmit");
#endif

    auto frameCommandBuffers_ = vk::raii::CommandBuffers(device.GetDevice(), {
        .commandPool = frameCommandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = framesInFlight,
    });
    for (uint32_t i = 0; i < framesInFlight; i++) {
        auto& frameContext = frameContexts.emplace_back(FrameContext {
            .commandBuffer = CommandBuffer(std::move(frameCommandBuffers_[i])),
            .presentCompleteSemaphore = vk::raii::Semaphore(device.GetDevice(), vk::SemaphoreCreateInfo()),
            .fence = vk::raii::Fence(device.GetDevice(), vk::FenceCreateInfo {
                .flags = vk::FenceCreateFlagBits::eSignaled,
            }),
        });
#if defined(DVK_ENABLE_DEBUG_LABELS)
        device.GetDevice().setDebugUtilsObjectNameEXT(*frameContext.commandBuffer.GetCommandBuffer(), "DVK Frame " + i);
        device.GetDevice().setDebugUtilsObjectNameEXT(*frameContext.presentCompleteSemaphore, "DVK Frame " + i);
        device.GetDevice().setDebugUtilsObjectNameEXT(*frameContext.fence, "DVK Frame " + i);
#endif
    }
}

const Device& CommandEncoder::GetDevice() const {
    return device;
}

const vk::raii::Semaphore& CommandEncoder::WaitForNextFrame() const {
    auto& frameContext = frameContexts[nextFrameIndex];

    if (device.GetDevice().waitForFences(*frameContext.fence, vk::True, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence!");
    }
    return frameContext.presentCompleteSemaphore;
}

std::tuple<uint32_t, CommandBuffer&> CommandEncoder::GetNextFrame() {
    uint32_t frameIndex = nextFrameIndex;
    auto& frameContext = frameContexts[frameIndex];

    frameContext.deleteQueue.clear();

    device.GetDevice().resetFences(*frameContext.fence);
    frameContext.commandBuffer.GetCommandBuffer().reset();
    frameContext.commandBuffer.GetCommandBuffer().begin({});

    nextFrameIndex = (nextFrameIndex + 1) % framesInFlight;
    return { frameIndex, frameContext.commandBuffer };
}

void CommandEncoder::SubmitFrame(uint32_t frameIndex, const vk::raii::Semaphore& willSignalOnRenderFinished) const {
    auto& frameContext = frameContexts[frameIndex];

    frameContext.commandBuffer.GetCommandBuffer().end();

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*frameContext.presentCompleteSemaphore,
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*frameContext.commandBuffer.GetCommandBuffer(),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*willSignalOnRenderFinished,
    };
    device.GetQueue().submit(submitInfo, *frameContext.fence);
}

void CommandEncoder::Execute(const std::function<void(CommandBuffer&)>& func, const std::string& label) const {
    auto commandBuffer = CommandBuffer(std::move(vk::raii::CommandBuffers(device.GetDevice(), {
        .commandPool = oneTimeSubmitCommandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    }).front()));
#if defined(DVK_ENABLE_DEBUG_LABELS)
    device.GetDevice().setDebugUtilsObjectNameEXT(*commandBuffer.GetCommandBuffer(), label);
#endif
    commandBuffer.GetCommandBuffer().begin({
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    });
    func(commandBuffer);
    commandBuffer.GetCommandBuffer().end();
    device.GetQueue().submit(vk::SubmitInfo {
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffer.GetCommandBuffer(),
    });
}

void CommandEncoder::ExecuteAndWait(const std::function<void(CommandBuffer&)>& func, const std::string& label) const {
    auto commandBuffer = CommandBuffer(std::move(vk::raii::CommandBuffers(device.GetDevice(), {
        .commandPool = oneTimeSubmitCommandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    }).front()));
#if defined(DVK_ENABLE_DEBUG_LABELS)
    device.GetDevice().setDebugUtilsObjectNameEXT(*commandBuffer.GetCommandBuffer(), label);
#endif
    commandBuffer.GetCommandBuffer().begin({
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    });
    func(commandBuffer);
    commandBuffer.GetCommandBuffer().end();
    device.GetQueue().submit(vk::SubmitInfo {
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffer.GetCommandBuffer(),
    });
    device.GetDevice().waitIdle();
}

void CommandEncoder::DropRefOnceNotInUse(uint32_t frameIndex, std::shared_ptr<void> dvkObject) {
    auto& frameContext = frameContexts[frameIndex];
    frameContext.deleteQueue.emplace_back(std::move(dvkObject));
}