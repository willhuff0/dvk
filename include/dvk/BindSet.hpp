#pragma once

#include "BindPool.hpp"
#include "BindSetLayout.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <memory>
#include <vector>
#include <string>

namespace dvk {
    class Device;
    class Buffer;
    class Binder;
    class BufferBinder;
    class ImageBinder;
    class SamplerBinder;

    class BindSet : public std::enable_shared_from_this<BindSet> {
    public:
        using Ref = std::shared_ptr<BindSet>;

        static Ref Create(const Device& device, const BindPool::Ref& pool, const BindSetLayout::Ref& layout, const std::string& label = "");
        explicit BindSet(vk::raii::DescriptorSet descriptorSet);

        [[nodiscard]] const vk::raii::DescriptorSet& GetDescriptorSet() const;

    private:
        vk::raii::DescriptorSet descriptorSet;
    };

    class Binder {
        friend class BufferBinder;
        friend class ImageBinder;
        friend class SamplerBinder;
    public:
        BufferBinder BindBuffer(uint32_t binding);
        ImageBinder BindImage(uint32_t binding);
        SamplerBinder BindSampler(uint32_t binding);

        void Update(const vk::raii::DescriptorSet& descriptorSet);
        void Update(const BindSet::Ref& bindSet);

    private:
        void addBufferWrite(vk::WriteDescriptorSet writeInfo, vk::DescriptorBufferInfo bufferInfo);
        void addImageWrite(vk::WriteDescriptorSet writeInfo, vk::DescriptorImageInfo imageInfo);

        std::vector<vk::DescriptorBufferInfo> bufferInfos;
        std::vector<vk::DescriptorImageInfo> imageInfos;
        std::vector<vk::WriteDescriptorSet> writes;
        std::vector<size_t> writeIndexToInfoIndex;
    };

    class BufferBinder {
    public:
        BufferBinder(Binder& binder, uint32_t binding);

        BufferBinder& SetBufferType(vk::DescriptorType type);
        BufferBinder& SetBuffer(const vk::raii::Buffer& buffer);
        BufferBinder& SetRange(vk::DeviceSize offset, vk::DeviceSize size);
        BufferBinder& SetArrayTarget(uint32_t index, uint32_t count);

        Binder& FinishBuffer() const;

    private:
        Binder& binder;
        vk::DescriptorBufferInfo bufferInfo;
        vk::WriteDescriptorSet writeInfo;
    };

    class ImageBinder {
    public:
        ImageBinder(Binder& binder, uint32_t binding);

        ImageBinder& SetImageType(vk::DescriptorType type);
        ImageBinder& SetImageView(const vk::raii::ImageView& view);
        ImageBinder& SetImageLayout(vk::ImageLayout layout);
        ImageBinder& SetCombinedSampler(const vk::raii::Sampler& sampler);
        ImageBinder& SetArrayTarget(uint32_t index, uint32_t count);

        Binder& FinishImage() const;

    private:
        Binder& binder;
        vk::DescriptorImageInfo imageInfo;
        vk::WriteDescriptorSet writeInfo;
    };

    class SamplerBinder {
    public:
        SamplerBinder(Binder& binder, uint32_t binding);

        SamplerBinder& SetSampler(const vk::raii::Sampler& sampler);
        SamplerBinder& SetArrayTarget(uint32_t index, uint32_t count);

        Binder& FinishSampler() const;

    private:
        Binder& binder;
        vk::DescriptorImageInfo imageInfo;
        vk::WriteDescriptorSet writeInfo;
    };
}