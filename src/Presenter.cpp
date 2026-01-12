#include <dvk/Presenter.hpp>
#include "dvk/Device.hpp"
#include "dvk/Surface.hpp"

#include <unordered_set>
#include <algorithm>
#include <limits>

using namespace dvk;

static vk::Extent2D chooseSwapChainExtent(const vk::SurfaceCapabilitiesKHR& capabilities, vk::Extent2D surfaceSize) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    return {
        std::clamp<uint32_t>(surfaceSize.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp<uint32_t>(surfaceSize.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

static vk::SurfaceFormatKHR chooseSwapChainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eR8G8B8A8Srgb) continue;
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb) continue;
        return availableFormat;
    }

    return availableFormats[0];
}

static vk::PresentModeKHR chooseSwapChainPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    std::unordered_set<vk::PresentModeKHR> presentModes;
    for (const auto& availablePresentMode : availablePresentModes) {
        presentModes.insert(availablePresentMode);
    }

    // if (presentModes.contains(vk::PresentModeKHR::eFifoLatestReady)) return vk::PresentModeKHR::eFifoLatestReady;
    // if (presentModes.contains(vk::PresentModeKHR::eFifoLatestReadyEXT)) return vk::PresentModeKHR::eFifoLatestReadyEXT;
    if (presentModes.contains(vk::PresentModeKHR::eMailbox)) return vk::PresentModeKHR::eMailbox;
    if (presentModes.contains(vk::PresentModeKHR::eFifoRelaxed)) return vk::PresentModeKHR::eFifoRelaxed;
    if (presentModes.contains(vk::PresentModeKHR::eFifo)) return vk::PresentModeKHR::eFifo;
    if (presentModes.contains(vk::PresentModeKHR::eImmediate)) return vk::PresentModeKHR::eImmediate;

    return availablePresentModes[0];
}

Presenter::Presenter(const Device& device, const Surface& surface, vk::Extent2D surfaceSize) : device(device) {
    Resize(surface, surfaceSize);
}

void Presenter::Resize(const Surface& surface, vk::Extent2D surfaceSize) {
    auto surfaceCapabilities = device.GetPhysicalDevice().getSurfaceCapabilitiesKHR(surface.GetSurface());
    auto [format, colorSpace] = chooseSwapChainSurfaceFormat(device.GetPhysicalDevice().getSurfaceFormatsKHR(surface.GetSurface()));
    auto presentMode = chooseSwapChainPresentMode(device.GetPhysicalDevice().getSurfacePresentModesKHR(surface.GetSurface()));
    extent = chooseSwapChainExtent(surfaceCapabilities, surfaceSize);
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    minImageCount = (surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount) ? surfaceCapabilities.maxImageCount : minImageCount;

    swapChain = vk::raii::SwapchainKHR(device.GetDevice(), {
        .flags = {},
        .surface = surface.GetSurface(),
        .minImageCount = minImageCount,
        .imageFormat = format,
        .imageColorSpace = colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = presentMode,
        .clipped = true,
        .oldSwapchain = nullptr,
    });
    swapChainImages = swapChain.getImages();
    imageFormat = format;

    swapChainImageViews.clear();
    renderFinishedSemaphores.clear();
    vk::ImageViewCreateInfo imageViewCreateInfo {
        .viewType = vk::ImageViewType::e2D,
        .format = imageFormat,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    for (auto& image : swapChainImages) {
        imageViewCreateInfo.image = image;
        swapChainImageViews.emplace_back(device.GetDevice(), imageViewCreateInfo);
        renderFinishedSemaphores.emplace_back(device.GetDevice(), vk::SemaphoreCreateInfo());
    }
}

vk::Format Presenter::GetFormat() const {
    return imageFormat;
}

vk::Extent2D Presenter::GetExtent() const {
    return extent;
}

std::tuple<PresenterResult, uint32_t, const vk::raii::ImageView&, const vk::raii::Semaphore&> Presenter::GetNextImage(const vk::raii::Semaphore& willSignalOnImageReady) const {
    auto [result, imageIndex] = swapChain.acquireNextImage(std::numeric_limits<uint64_t>::max(), willSignalOnImageReady, nullptr);
    if (result == vk::Result::eErrorOutOfDateKHR) return { PresenterResult::Fail_MustResize, ~0, nullptr, nullptr };
    return { PresenterResult::Success, imageIndex, swapChainImageViews[imageIndex], renderFinishedSemaphores[imageIndex] };
}

void Presenter::PrepareImageForRendering(const vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex) const {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .srcAccessMask = {},
        .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .image = swapChainImages.at(imageIndex),
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    commandBuffer.pipelineBarrier2({
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    });
}
void Presenter::PrepareImageForPresentation(const vk::raii::CommandBuffer& commandBuffer, uint32_t imageIndex) const {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe,
        .dstAccessMask = {},
        .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .newLayout = vk::ImageLayout::ePresentSrcKHR,
        .image = swapChainImages.at(imageIndex),
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    commandBuffer.pipelineBarrier2({
        .dependencyFlags = {},
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    });
}

PresenterResult Presenter::Present(uint32_t imageIndex) const {
    try {
        auto result = device.GetQueue().presentKHR({
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &*renderFinishedSemaphores[imageIndex],
            .swapchainCount = 1,
            .pSwapchains = &*swapChain,
            .pImageIndices = &imageIndex,
        });
        switch (result) {
            case vk::Result::eSuboptimalKHR:
            case vk::Result::eErrorOutOfDateKHR:
                return PresenterResult::Fail_MustResize;
            default:
                break;
        }
    } catch (const vk::SystemError& e) {
        if (e.code().value() == static_cast<int>(vk::Result::eErrorOutOfDateKHR)) return PresenterResult::Fail_MustResize;
        throw;
    }

    return PresenterResult::Success;
}
