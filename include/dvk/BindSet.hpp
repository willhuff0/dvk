#pragma once

#include <dvk/Config.hpp>
#include "Device.hpp"
#include "BindPool.hpp"
#include "BindSetLayout.hpp"

#include "Buffer.hpp"
#include "TextureView.hpp"
#include "Sampler.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <memory>
#include <vector>
#include <string>
#include <array>
#include <unordered_map>

namespace dvk {
    class Device;
    class Binder;
    class BufferBinder;
    class TextureBinder;
    class SamplerBinder;

    class BindSet : public std::enable_shared_from_this<BindSet> {
    public:
        using Ref = std::shared_ptr<BindSet>;
        template<size_t N>
        using Refs = std::array<Ref, N>;

        static Ref Create(const Device& device, const BindPool::Ref& pool, const BindSetLayout::Ref& layout, const std::string& label = "");
        template<size_t N>
        static Refs<N> Create(const Device& device, const BindPool::Ref& pool, const BindSetLayout::Ref& layout, const std::string& label = "");
        explicit BindSet(vk::raii::DescriptorSet descriptorSet);

        [[nodiscard]] const vk::raii::DescriptorSet& GetDescriptorSet() const;

    private:
        vk::raii::DescriptorSet descriptorSet;
    };

    class Binder {
        friend class BufferBinder;
        friend class TextureBinder;
        friend class SamplerBinder;
    public:
        BufferBinder BindBuffer(uint32_t binding);
        TextureBinder BindTexture(uint32_t binding);
        SamplerBinder BindSampler(uint32_t binding);

        void Update(const vk::raii::DescriptorSet& descriptorSet);
        void Update(const BindSet::Ref& bindSet);

    private:
        void addBufferWrite(vk::WriteDescriptorSet writeInfo, vk::DescriptorBufferInfo bufferInfo, Buffer::Ref buffer);
        void addImageWrite(vk::WriteDescriptorSet writeInfo, vk::DescriptorImageInfo imageInfo, TextureView::Ref textureView, Sampler::Ref sampler);

        std::unordered_map<uint32_t, vk::DescriptorBufferInfo> bufferInfos;
        std::unordered_map<uint32_t, Buffer::Ref> buffers;
        std::unordered_map<uint32_t, vk::DescriptorImageInfo> imageInfos;
        std::unordered_map<uint32_t, TextureView::Ref> textureViews;
        std::unordered_map<uint32_t, Sampler::Ref> samplers;
        std::unordered_map<uint32_t, vk::WriteDescriptorSet> writes;
    };

    class BufferBinder {
    public:
        BufferBinder(Binder& binder, uint32_t binding);

        BufferBinder& SetBufferType(vk::DescriptorType type);
        BufferBinder& SetBuffer(Buffer::Ref buffer);
        BufferBinder& SetRange(vk::DeviceSize offset, vk::DeviceSize size);
        BufferBinder& SetArrayTarget(uint32_t index, uint32_t count);

        Binder& FinishBuffer() const;

    private:
        Binder& binder;
        vk::DescriptorBufferInfo bufferInfo;
        vk::WriteDescriptorSet writeInfo;
        Buffer::Ref buffer;
    };

    class TextureBinder {
    public:
        TextureBinder(Binder& binder, uint32_t binding);

        TextureBinder& SetTextureType(vk::DescriptorType type);
        TextureBinder& SetTextureView(TextureView::Ref textureView);
        TextureBinder& SetImageLayout(vk::ImageLayout layout);
        TextureBinder& SetCombinedSampler(Sampler::Ref sampler);
        TextureBinder& SetArrayTarget(uint32_t index, uint32_t count);

        Binder& FinishTexture() const;

    private:
        Binder& binder;
        vk::DescriptorImageInfo imageInfo;
        vk::WriteDescriptorSet writeInfo;
        TextureView::Ref textureView;
        Sampler::Ref sampler;
    };

    class SamplerBinder {
    public:
        SamplerBinder(Binder& binder, uint32_t binding);

        SamplerBinder& SetSampler(Sampler::Ref sampler);
        SamplerBinder& SetArrayTarget(uint32_t index, uint32_t count);

        Binder& FinishSampler() const;

    private:
        Binder& binder;
        vk::DescriptorImageInfo imageInfo;
        vk::WriteDescriptorSet writeInfo;
        Sampler::Ref sampler;
    };

    template<size_t N>
    BindSet::Refs<N> BindSet::Create(const Device& device, const BindPool::Ref& pool, const BindSetLayout::Ref& layout, const std::string& label) {
        std::array<Ref, N> descriptorSets;
        std::array<vk::DescriptorSetLayout, N> layouts;
        layouts.fill(*layout->GetDescriptorSetLayout());

        auto descriptorSetsVector = vk::raii::DescriptorSets(device.GetDevice(), {
            .descriptorPool = pool->GetDescriptorPool(),
            .descriptorSetCount = N,
            .pSetLayouts = layouts.data(),
        });

        for (size_t i = 0; i < N; i++) {
#if defined(DVK_ENABLE_DEBUG_LABELS)
            if (!label.empty()) device.GetDevice().setDebugUtilsObjectNameEXT(*descriptorSetsVector[i], label + " (" + std::to_string(N) + ")");
#endif

            descriptorSets[i] = std::make_shared<BindSet>(std::move(descriptorSetsVector[i]));
        }

        return descriptorSets;
    }
}
