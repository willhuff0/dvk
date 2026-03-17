#include <dvk/CommandEncoder.hpp>
#include <dvk/Config.hpp>
#include "dvk/Device.hpp"
#include "dvk/CommandBuffer.hpp"

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
        frameCommandBuffers.emplace_back(std::move(frameCommandBuffers_[i]));
        framePresentCompeteSemaphores.emplace_back(device.GetDevice(), vk::SemaphoreCreateInfo());
        frameFences.emplace_back(device.GetDevice(), vk::FenceCreateInfo {
            .flags = vk::FenceCreateFlagBits::eSignaled,
        });
#if defined(DVK_ENABLE_DEBUG_LABELS)
        device.GetDevice().setDebugUtilsObjectNameEXT(*frameCommandBuffers[i].GetCommandBuffer(), "DVK Frame " + i);
        device.GetDevice().setDebugUtilsObjectNameEXT(*framePresentCompeteSemaphores[i], "DVK Frame " + i);
        device.GetDevice().setDebugUtilsObjectNameEXT(*frameFences[i], "DVK Frame " + i);
#endif
    }
}

const Device& CommandEncoder::GetDevice() const {
    return device;
}

const vk::raii::Semaphore& CommandEncoder::WaitForNextFrame() const {
    if (device.GetDevice().waitForFences(*frameFences[nextFrameIndex], vk::True, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence!");
    }
    return framePresentCompeteSemaphores[nextFrameIndex];
}

std::tuple<uint32_t, CommandBuffer&> CommandEncoder::GetNextFrame() {
    uint32_t frameIndex = nextFrameIndex;

    device.GetDevice().resetFences(*frameFences[frameIndex]);
    frameCommandBuffers[frameIndex].GetCommandBuffer().reset();
    frameCommandBuffers[frameIndex].GetCommandBuffer().begin({});

    nextFrameIndex = (nextFrameIndex + 1) % framesInFlight;
    return { frameIndex, frameCommandBuffers[frameIndex] };
}

void CommandEncoder::SubmitFrame(uint32_t frameIndex, const vk::raii::Semaphore& willSignalOnRenderFinished) const {
    frameCommandBuffers[frameIndex].GetCommandBuffer().end();

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*framePresentCompeteSemaphores[frameIndex],
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*frameCommandBuffers[frameIndex].GetCommandBuffer(),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*willSignalOnRenderFinished,
    };
    device.GetQueue().submit(submitInfo, *frameFences[frameIndex]);
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
