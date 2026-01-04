#pragma once

#include <dvk/Config.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <memory>
#include <string>

namespace dvk {
    class Device;

    class Sampler {
    public:
        class Builder;
        using Ref = std::shared_ptr<Sampler>;

        static Builder Create(const Device& device);
        explicit Sampler(vk::raii::Sampler sampler);

        [[nodiscard]] const vk::raii::Sampler& GetSampler() const;

    private:
        vk::raii::Sampler sampler = nullptr;

    public:
        class Builder {
        public:
            explicit Builder(const Device& device);

            Builder& SetLabel(const std::string& label);
            Builder& SetFilter(vk::Filter filter);
            Builder& SetFilter(vk::Filter min, vk::Filter mag);
            Builder& SetMipMapFilter(vk::SamplerMipmapMode filter);
            Builder& SetAddressMode(vk::SamplerAddressMode addressMode);
            Builder& SetAddressMode(vk::SamplerAddressMode u, vk::SamplerAddressMode v, vk::SamplerAddressMode w);
            Builder& SetMipLodBias(float bias);
            Builder& SetAnisotropy(bool enabled, float max);
            Builder& SetCompare(bool enabled, vk::CompareOp compareOp);
            Builder& SetLodBounds(float minLod, float maxLod);
            Builder& SetBorderColor(vk::BorderColor borderColor);
            Builder& UseUnnormalizedCoords(bool unnormalizedCoords = true);

            [[nodiscard]] Ref Build() const;

        private:
            const Device& device;
            vk::SamplerCreateInfo samplerInfo;
#if defined(DVK_ENABLE_DEBUG_LABELS)
            std::string label;
#endif
        };
    };
}