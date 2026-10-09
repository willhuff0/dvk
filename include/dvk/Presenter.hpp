#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <vector>
#include <tuple>

namespace dvk {
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

        [[nodiscard]] vk::Format GetFormat() const;
        [[nodiscard]] vk::Extent2D GetExtent() const;
        [[nodiscard]] uint32_t GetMinImageCount() const;
        [[nodiscard]] uint32_t GetSwapChainImageCount() const;

        [[nodiscard]] std::tuple<PresenterResult, uint32_t, const vk::raii::ImageView&, const vk::raii::Semaphore&> GetNextImage(const vk::raii::Semaphore& willSignalOnImageReady) const;
        void PrepareImageForRendering(CommandBuffer& commandBuffer, uint32_t imageIndex) const;
        void PrepareImageForPresentation(CommandBuffer& commandBuffer, uint32_t imageIndex) const;
        [[nodiscard]] PresenterResult Present(uint32_t imageIndex) const;

    private:
        const Device& device;

        vk::Format imageFormat = vk::Format::eUndefined;
        vk::Extent2D extent;
        uint32_t minImageCount;

        vk::raii::SwapchainKHR swapChain = nullptr;
        std::vector<vk::Image> swapChainImages;
        std::vector<vk::raii::ImageView> swapChainImageViews;
        std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    };
}