#pragma once

#include <dvk/Config.hpp>

#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc_raii.hpp>

#include <memory>
#include <string>

namespace dvk {
    class Device;
    class CommandEncoder;
    class CommandBuffer;

    class Texture {
    public:
        class Builder;
        using Ref = std::shared_ptr<Texture>;

        static Builder Create(const Device& device);
#if defined(DVK_ENABLE_DEBUG_LABELS)
        Texture(vma::raii::Image image, vk::Extent3D extent, vk::Format format, std::string label);
#else
        Texture(vma::raii::Image image, vk::Extent3D extent, vk::Format format);
#endif

        [[nodiscard]] const vma::raii::Image& GetImage() const;
        [[nodiscard]] vk::Extent3D GetExtent() const;
        [[nodiscard]] vk::Format GetFormat() const;
#if defined(DVK_ENABLE_DEBUG_LABELS)
        [[nodiscard]] const std::string& GetLabel() const;
#endif

        void WriteWithStaging(const CommandEncoder& commandEncoder, const void* src, vk::DeviceSize size, uint32_t mipLevel = 0, uint32_t arrayLayer = 0);

        void SimpleLayoutBarrier(CommandBuffer& commandBuffer, vk::ImageLayout from, vk::ImageLayout to);

    private:
        vma::raii::Image image;
        vk::Extent3D extent;
        vk::Format format;
#if defined(DVK_ENABLE_DEBUG_LABELS)
        std::string label;
#endif

    public:
        class Builder {
        public:
            explicit Builder(const Device& device);

            Builder& SetLabel(const std::string& label);
            Builder& SetType(vk::ImageType type);
            Builder& SetFormat(vk::Format format);
            Builder& SetExtent(vk::Extent3D extent);
            Builder& SetMipLevelCount(uint32_t mipLevelCount);
            Builder& SetArrayLayerCount(uint32_t arrayLayerCount);
            Builder& SetSampleCount(vk::SampleCountFlagBits sampleCount);
            Builder& SetUsage(vk::ImageUsageFlags usage);
            Builder& AllowCubeViews(bool cubeViews = true);
            Builder& Allow2DArrayViews(bool arrayViews = true);
            Builder& EnableWriteWithStaging();

            [[nodiscard]] Ref Build() const;

        private:
            const Device& device;
            vk::ImageCreateInfo imageInfo;
#if defined(DVK_ENABLE_DEBUG_LABELS)
            std::string label;
#endif
        };
    };
}
