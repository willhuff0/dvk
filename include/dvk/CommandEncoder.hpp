#pragma once

#include "dvk/CommandBuffer.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <vector>
#include <tuple>
#include <functional>
#include <memory>

namespace dvk {
    class Device;

    class CommandEncoder {
    public:
        explicit CommandEncoder(const Device& device, uint32_t framesInFlight = 2);

        [[nodiscard]] const Device& GetDevice() const;

        const vk::raii::Semaphore& WaitForNextFrame() const;
        [[nodiscard]] std::tuple<uint32_t, CommandBuffer&> GetNextFrame();
        void SubmitFrame(uint32_t frameIndex, const vk::raii::Semaphore& willSignalOnRenderFinished) const;

        void ExecuteAndWait(const std::function<void(CommandBuffer&)>& func, const std::string& label = "DVK ExecuteAndWait") const;

        void DropRefOnceNotInUse(uint32_t frameIndex, std::shared_ptr<void> dvkObject);

    private:
        struct FrameContext {
            CommandBuffer commandBuffer;
            vk::raii::Semaphore presentCompleteSemaphore;
            vk::raii::Fence fence;
            std::vector<std::shared_ptr<void>> deleteQueue;
        };

        const Device& device;

        uint32_t framesInFlight;
        uint32_t nextFrameIndex = 0;

        vk::raii::CommandPool frameCommandPool = nullptr;
        std::vector<FrameContext> frameContexts;

        vk::raii::CommandPool oneTimeSubmitCommandPool = nullptr;
    };
}