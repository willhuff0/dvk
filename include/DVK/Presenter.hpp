#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <vector>
#include <tuple>

namespace DVK {
    class Surface;
    class Device;
    class CommandBuffer;

    enum class PresenterResult {
        Success,
        Fail_MustResize,
    };

    class Presenter {
    public:
        Presenter(const Device& device, const Surface& surface, vk::Extent2D surfaceSize);

        void Resize(const Surface& surface, vk::Extent2D surfaceSize);

        [[nodiscard]] std::tuple<PresenterResult, uint32_t> GetNextImage(const vk::raii::Semaphore& willSignalOnImageReady) const;
        void PrepareImageForRendering(const CommandBuffer& commandBuffer, uint32_t imageIndex) const;
        void PrepareImageForPresentation(const CommandBuffer& commandBuffer, uint32_t imageIndex) const;
        [[nodiscard]] PresenterResult Present(uint32_t imageIndex, const vk::raii::Semaphore& willWaitToPresent) const;

    private:
        const Device& device;

        vk::Format imageFormat = vk::Format::eUndefined;
        vk::Extent2D extent;

        vk::raii::SwapchainKHR swapChain = nullptr;
        std::vector<vk::Image> swapChainImages;
        std::vector<vk::raii::ImageView> swapChainImageViews;
    };
}