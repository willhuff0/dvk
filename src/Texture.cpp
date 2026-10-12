#include <dvk/Texture.hpp>
#include "dvk/CommandEncoder.hpp"

#include "dvk/Buffer.hpp"

#include <utility>

#include "dvk/CommandBuffer.hpp"

using namespace dvk;

static void copyBufferToImage(
    const vk::raii::CommandBuffer& commandBuffer,
    const vk::raii::Buffer& buffer,
    const vk::raii::Image& image,
    vk::Extent3D extent,
    uint32_t mipLevel,
    uint32_t arrayLayer
) {
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

static void blitImage(
    const vk::raii::CommandBuffer& commandBuffer,
    vk::raii::Image& from,
    vk::raii::Image& to,
    vk::Extent3D extent,
    uint32_t fromMipLevel,
    uint32_t toMipLevel,
    uint32_t arrayLayer
) {
    std::array srcOffsets = {
        vk::Offset3D {0, 0, 0},
        vk::Offset3D {static_cast<int32_t>(extent.width >> fromMipLevel), static_cast<int32_t>(extent.height >> fromMipLevel), 1}
    };
    std::array dstOffsets = {
        vk::Offset3D {0, 0, 0},
        vk::Offset3D {static_cast<int32_t>(extent.width >> toMipLevel), static_cast<int32_t>(extent.height >> toMipLevel), 1}
    };

    vk::ImageBlit imageBlit {
        .srcSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = fromMipLevel,
            .baseArrayLayer = arrayLayer,
            .layerCount = 1,
        },
        .srcOffsets = srcOffsets,
        .dstSubresource = {
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = toMipLevel,
            .baseArrayLayer = arrayLayer,
            .layerCount = 1,
        },
        .dstOffsets = dstOffsets,
    };
    commandBuffer.blitImage(
        from,
        vk::ImageLayout::eTransferSrcOptimal,
        to,
        vk::ImageLayout::eTransferDstOptimal,
        imageBlit,
        vk::Filter::eLinear
    );
}

// Texture
Texture::Builder Texture::Create(const Device& device) {
    return Builder(device);
}

#if defined(DVK_ENABLE_DEBUG_LABELS)
Texture::Texture(vma::raii::Image image, vk::Extent3D extent, vk::Format format, std::string label)
    : image(std::move(image)), extent(extent), format(format), label(std::move(label)) { }
#else
Texture::Texture(vma::raii::Image image, vk::Extent3D extent, vk::Format format)
    : image(std::move(image)), extent(extent), format(format) { }
#endif

const vma::raii::Image& Texture::GetImage() const {
    return image;
}

vk::Extent3D Texture::GetExtent() const {
    return extent;
}

vk::Format Texture::GetFormat() const {
    return format;
}

#if defined(DVK_ENABLE_DEBUG_LABELS)
const std::string& Texture::GetLabel() const {
    return label;
}
#endif

void Texture::WriteWithStaging(
    const CommandEncoder& commandEncoder,
    const void* src,
    vk::DeviceSize size,
    uint32_t mipLevel,
    uint32_t arrayLayer
) {
    auto stagingBuffer = Buffer::CreateStagingBuffer(commandEncoder.GetDevice(), size);
    std::memcpy(stagingBuffer->GetMappedData(), src, size);

    commandEncoder.ExecuteAndWait([&](CommandBuffer& commandBuffer) {
        commandBuffer.InsertBarriers()
            .AddImageBarrier()
                .SetImage(image)
                .SetLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal)
                .InferStagesAndAccess()
                .SetMipLevels(mipLevel)
                .SetArrayLayers(arrayLayer)
                .FinishImageBarrier()
            .FinishBarriers();
        copyBufferToImage(commandBuffer.GetCommandBuffer(), stagingBuffer->GetBuffer(), image, extent, mipLevel, arrayLayer);
        commandBuffer.InsertBarriers()
            .AddImageBarrier()
                .SetImage(image)
                .SetLayout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal)
                .InferStagesAndAccess()
                .SetMipLevels(mipLevel)
                .SetArrayLayers(arrayLayer)
                .FinishImageBarrier()
            .FinishBarriers();
    }, "DVK Texture WriteWithStaging");
}

void Texture::WriteWithStagingAndGenerateMipMaps(
    const CommandEncoder& commandEncoder,
    const void* src,
    vk::DeviceSize size,
    uint32_t mipMapCount,
    uint32_t arrayLayer
) {
    auto stagingBuffer = Buffer::CreateStagingBuffer(commandEncoder.GetDevice(), size);
    std::memcpy(stagingBuffer->GetMappedData(), src, size);

    commandEncoder.ExecuteAndWait([&](CommandBuffer& commandBuffer) {
        commandBuffer.InsertBarriers()
            .AddImageBarrier()
                .SetImage(image)
                .SetLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal)
                .SetStages(vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eTransfer)
                .SetAccess({}, vk::AccessFlagBits2::eTransferWrite)
                .SetMipLevels(0, mipMapCount)
                .SetArrayLayers(arrayLayer)
                .FinishImageBarrier()
            .FinishBarriers();
        copyBufferToImage(commandBuffer.GetCommandBuffer(), stagingBuffer->GetBuffer(), image, extent, 0, arrayLayer);
        commandBuffer.InsertBarriers()
            .AddImageBarrier()
                .SetImage(image)
                .SetLayout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal)
                .SetStages(vk::PipelineStageFlagBits2::eTransfer, vk::PipelineStageFlagBits2::eTransfer)
                .SetAccess(vk::AccessFlagBits2::eTransferWrite, vk::AccessFlagBits2::eTransferRead)
                .SetMipLevels(0, mipMapCount)
                .SetArrayLayers(arrayLayer)
                .FinishImageBarrier()
            .AddImageBarrier()
                .SetImage(image)
                .SetLayout(vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal)
                .SetStages(vk::PipelineStageFlagBits2::eTopOfPipe, vk::PipelineStageFlagBits2::eTransfer)
                .SetAccess({}, vk::AccessFlagBits2::eTransferWrite)
                .SetMipLevels(1, mipMapCount - 1)
                .SetArrayLayers(arrayLayer)
                .FinishImageBarrier()
            .FinishBarriers();
        for (uint32_t i = 1; i < mipMapCount; i++) {
            blitImage(
                commandBuffer.GetCommandBuffer(),
                image,
                image,
                GetExtent(),
                i - 1,
                i,
                arrayLayer
            );
            commandBuffer.InsertBarriers()
                .AddImageBarrier()
                    .SetImage(image)
                    .SetLayout(vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal)
                    .SetStages(vk::PipelineStageFlagBits2::eTransfer, vk::PipelineStageFlagBits2::eTransfer)
                    .SetAccess(vk::AccessFlagBits2::eTransferWrite, vk::AccessFlagBits2::eTransferRead)
                    .SetMipLevels(i)
                    .SetArrayLayers(arrayLayer)
                    .FinishImageBarrier()
                .FinishBarriers();
        }
        commandBuffer.InsertBarriers()
            .AddImageBarrier()
                .SetImage(image)
                .SetLayout(vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eShaderReadOnlyOptimal)
                .SetStages(vk::PipelineStageFlagBits2::eTransfer, vk::PipelineStageFlagBits2::eFragmentShader)
                .SetAccess(vk::AccessFlagBits2::eTransferRead, vk::AccessFlagBits2::eShaderRead)
                .SetMipLevels(0, mipMapCount)
                .SetArrayLayers(arrayLayer)
                .FinishImageBarrier()
            .FinishBarriers();

    }, "DVK Texture WriteWithStagingAndGenerateMipMaps");
}

void Texture::SimpleLayoutBarrier(CommandBuffer& commandBuffer, vk::ImageLayout from, vk::ImageLayout to) {
    commandBuffer.InsertBarriers()
        .AddImageBarrier()
            .SetImage(image)
            .SetLayout(from, to)
            .InferStagesAndAccess()
            .FinishImageBarrier()
        .FinishBarriers();
}

// Texture Builder
Texture::Builder::Builder(const Device& device) : device(device) {
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
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

Texture::Builder& Texture::Builder::AllowCubeViews(bool cubeViews) {
    if (imageInfo.arrayLayers < 6) imageInfo.arrayLayers = 6;
    if (cubeViews) {
        imageInfo.flags |= vk::ImageCreateFlagBits::eCubeCompatible;
    } else {
        imageInfo.flags &= ~vk::ImageCreateFlagBits::eCubeCompatible;
    }
    return *this;
}

Texture::Builder& Texture::Builder::Allow2DArrayViews(bool arrayViews) {
    if (arrayViews) {
        imageInfo.flags |= vk::ImageCreateFlagBits::e2DArrayCompatible;
    } else {
        imageInfo.flags &= ~vk::ImageCreateFlagBits::e2DArrayCompatible;
    }
    return *this;
}

Texture::Builder& Texture::Builder::EnableWriteWithStaging() {
    imageInfo.usage |= vk::ImageUsageFlagBits::eTransferDst;
    return *this;
}

Texture::Ref Texture::Builder::Build() const {
    auto image = vma::raii::Image(device.GetAllocator(), imageInfo, vma::AllocationCreateInfo {
        .usage = vma::MemoryUsage::eAutoPreferDevice,
    });

#if defined(DVK_ENABLE_DEBUG_LABELS)
    if (!label.empty()) device.GetDevice().setDebugUtilsObjectNameEXT(*image, label);
    return std::make_shared<Texture>(std::move(image), imageInfo.extent, imageInfo.format, label);
#else
    return std::make_shared<Texture>(std::move(image), imageInfo.extent, imageInfo.format);
#endif
}
