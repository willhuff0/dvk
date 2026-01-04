#include <dvk/CommandEncoder.hpp>
#include "dvk/Device.hpp"

#include <stdexcept>

using namespace dvk;

CommandEncoder::CommandEncoder(const Device& device, uint32_t framesInFlight)
    : device(device), framesInFlight(framesInFlight) {
    frameCommandPool = vk::raii::CommandPool(device.GetDevice(), {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = device.GetQueueFamilyIndex(),
    });

    oneTimeSubmitCommandPool = vk::raii::CommandPool(device.GetDevice(), {
        .flags = vk::CommandPoolCreateFlagBits::eTransient,
        .queueFamilyIndex = device.GetQueueFamilyIndex(),
    });

    frameCommandBuffers = vk::raii::CommandBuffers(device.GetDevice(), {
        .commandPool = frameCommandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = framesInFlight,
    });
    for (uint32_t i = 0; i < framesInFlight; i++) {
        framePresentCompeteSemaphores.emplace_back(device.GetDevice(), vk::SemaphoreCreateInfo());
        frameFences.emplace_back(device.GetDevice(), vk::FenceCreateInfo {
            .flags = vk::FenceCreateFlagBits::eSignaled,
        });
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

std::tuple<uint32_t, const vk::raii::CommandBuffer&> CommandEncoder::GetNextFrame() {
    uint32_t frameIndex = nextFrameIndex;

    device.GetDevice().resetFences(*frameFences[frameIndex]);
    frameCommandBuffers[frameIndex].reset();
    frameCommandBuffers[frameIndex].begin({});

    nextFrameIndex = (nextFrameIndex + 1) % framesInFlight;
    return std::make_tuple(frameIndex, frameCommandBuffers[frameIndex]);
}

void CommandEncoder::SubmitFrame(uint32_t frameIndex, const vk::raii::Semaphore& willSignalOnRenderFinished) const {
    frameCommandBuffers[frameIndex].end();

    vk::PipelineStageFlags waitDestinationStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    const vk::SubmitInfo submitInfo {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*framePresentCompeteSemaphores[frameIndex],
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*frameCommandBuffers[frameIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*willSignalOnRenderFinished,
    };
    device.GetQueue().submit(submitInfo, *frameFences[frameIndex]);
}

void CommandEncoder::ExecuteOneTime(const std::function<void(const vk::raii::CommandBuffer&)>& func) const {
    auto commandBuffer = std::move(vk::raii::CommandBuffers(device.GetDevice(), {
        .commandPool = oneTimeSubmitCommandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1,
    }).front());
    commandBuffer.begin({
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    });
    func(commandBuffer);
    commandBuffer.end();
    device.GetQueue().submit(vk::SubmitInfo {
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffer,
    });
    device.GetDevice().waitIdle();
}
