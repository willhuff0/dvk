#include <dvk/Texture.hpp>
#include "dvk/Device.hpp"

#include "dvk/Buffer.hpp"
#include "dvk/CommandEncoder.hpp"

#include <utility>

using namespace dvk;

static void transitionImageLayout(const vk::raii::CommandBuffer& commandBuffer, const vk::raii::Image& image, vk::ImageLayout from, vk::ImageLayout to) {
    vk::ImageMemoryBarrier barrier {
        .oldLayout = from,
        .newLayout = to,
        .image = image,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = 0,
            .levelCount = vk::RemainingMipLevels,
            .baseArrayLayer = 0,
            .layerCount = vk::RemainingArrayLayers,
        },
    };

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (from == vk::ImageLayout::eUndefined && to == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (from == vk::ImageLayout::eTransferDstOptimal && to == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (from == vk::ImageLayout::eUndefined && to == vk::ImageLayout::eColorAttachmentOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        sourceStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        destinationStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    } else if (from == vk::ImageLayout::eColorAttachmentOptimal && to == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (from == vk::ImageLayout::eShaderReadOnlyOptimal && to == vk::ImageLayout::eColorAttachmentOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        sourceStage = vk::PipelineStageFlagBits::eFragmentShader;
        destinationStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    } else {
        throw std::invalid_argument("Unsupported layout transition!");
    }

    commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, nullptr, barrier);
}

static void copyBufferToImage(const vk::raii::CommandBuffer& commandBuffer, const vk::raii::Buffer& buffer, const vk::raii::Image& image, vk::Extent3D extent, uint32_t mipLevel, uint32_t arrayLayer) {
    vk::BufferImageCopy region {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = mipLevel,
            .baseArrayLayer = arrayLayer,
            .layerCount = 1,
        },
        .imageOffset = { 0, 0, 0 },
        .imageExtent = extent,
    };
    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, { region });
}

// Texture
Texture::Builder Texture::Create(const Device& device) {
    return Builder(device);
}

Texture::Texture(vma::raii::Image image, vk::Extent3D extent, vk::Format format)
    : image(std::move(image)), extent(extent), format(format) { }

const vma::raii::Image& Texture::GetImage() const {
    return image;
}

vk::Extent3D Texture::GetExtent() const {
    return extent;
}

vk::Format Texture::GetFormat() const {
    return format;
}

void Texture::Write(const CommandEncoder& commandEncoder, void* src, vk::DeviceSize size, uint32_t mipLevel, uint32_t arrayLayer) {
    auto stagingBuffer = Buffer::CreateStagingBuffer(commandEncoder.GetDevice(), size);
    std::memcpy(stagingBuffer->GetMappedData(), src, size);

    commandEncoder.ExecuteOneTime([&](const auto& commandBuffer) {
        transitionImageLayout(commandBuffer, image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
        copyBufferToImage(commandBuffer, stagingBuffer->GetBuffer(), image, extent, mipLevel, arrayLayer);
        transitionImageLayout(commandBuffer, image, vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);
    });
}

void Texture::TransitionLayout(const vk::raii::CommandBuffer& commandBuffer, vk::ImageLayout from, vk::ImageLayout to) const {
    transitionImageLayout(commandBuffer, image, from, to);
}

void Texture::PrepareForDepthAttachment(const vk::raii::CommandBuffer& commandBuffer) const {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        .srcAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        .dstStageMask = vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        .dstAccessMask = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        .oldLayout = vk::ImageLayout::eUndefined,
        .newLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .image = image,
        .subresourceRange = {
            .aspectMask = vk::ImageAspectFlagBits::eDepth,
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

// Texture Builder
Texture::Builder::Builder(const Device& device) : device(device) {
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
}

Texture::Builder& Texture::Builder::SetLabel(const std::string& label_) {
#if defined(DVK_ENABLE_DEBUG_LABELS)
    label = label_;
#endif
    return *this;
}

Texture::Builder& Texture::Builder::SetType(vk::ImageType type) {
    imageInfo.imageType = type;
    return *this;
}

Texture::Builder& Texture::Builder::SetFormat(vk::Format format) {
    imageInfo.format = format;
    return *this;
}

Texture::Builder& Texture::Builder::SetExtent(vk::Extent3D extent) {
    imageInfo.extent = extent;
    return *this;
}

Texture::Builder& Texture::Builder::SetMipLevelCount(uint32_t mipLevelCount) {
    imageInfo.mipLevels = mipLevelCount;
    return *this;
}

Texture::Builder& Texture::Builder::SetArrayLayerCount(uint32_t arrayLayerCount) {
    imageInfo.arrayLayers = arrayLayerCount;
    return *this;
}

Texture::Builder& Texture::Builder::SetSampleCount(vk::SampleCountFlagBits sampleCount) {
    imageInfo.samples = sampleCount;
    return *this;
}

Texture::Builder& Texture::Builder::SetUsage(vk::ImageUsageFlags usage) {
    imageInfo.usage = usage;
    return *this;
}

Texture::Builder& Texture::Builder::EnableWrite() {
    imageInfo.usage |= vk::ImageUsageFlagBits::eTransferSrc;
    return *this;
}

Texture::Ref Texture::Builder::Build() const {
    auto image = vma::raii::Image(device.GetAllocator(), imageInfo, vma::AllocationCreateInfo {
        .usage = vma::MemoryUsage::eAutoPreferDevice,
    });

#if defined(DVK_ENABLE_DEBUG_LABELS)
    if (!label.empty()) device.GetDevice().setDebugUtilsObjectNameEXT(*image, label);
#endif

    return std::make_shared<Texture>(std::move(image), imageInfo.extent, imageInfo.format);
}
