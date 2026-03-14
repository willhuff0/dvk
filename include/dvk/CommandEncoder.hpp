#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <vector>
#include <tuple>
#include <functional>

namespace dvk {
    class Device;
    class CommandBuffer;

    class CommandEncoder {
    public:
        explicit CommandEncoder(const Device& device, uint32_t framesInFlight = 2);

        [[nodiscard]] const Device& GetDevice() const;

        const vk::raii::Semaphore& WaitForNextFrame() const;
        [[nodiscard]] std::tuple<uint32_t, CommandBuffer&> GetNextFrame();
        void SubmitFrame(uint32_t frameIndex, const vk::raii::Semaphore& willSignalOnRenderFinished) const;

        void Execute(const std::function<void(CommandBuffer&)>& func) const;
        void ExecuteAndWait(const std::function<void(CommandBuffer&)>& func) const;

    private:
        const Device& device;

        uint32_t framesInFlight;
        uint32_t nextFrameIndex = 0;

        vk::raii::CommandPool frameCommandPool = nullptr;
        std::vector<CommandBuffer> frameCommandBuffers;
        std::vector<vk::raii::Semaphore> framePresentCompeteSemaphores;
        std::vector<vk::raii::Fence> frameFences;

        vk::raii::CommandPool oneTimeSubmitCommandPool = nullptr;
    };
}