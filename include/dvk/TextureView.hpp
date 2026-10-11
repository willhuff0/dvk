#pragma once

#include <dvk/Config.hpp>
#include "Texture.hpp"
#include "Device.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <memory>
#include <string>

namespace dvk {
    class Device;

    class TextureView {
    public:
        class Builder;
        using Ref = std::shared_ptr<TextureView>;
        template<size_t N>
        using Refs = std::array<Ref, N>;

        static Builder Create(const Device& device, Texture::Ref texture);
        explicit TextureView(Texture::Ref texture, vk::raii::ImageView imageView);

        [[nodiscard]] const Texture::Ref& GetTexture() const;
        [[nodiscard]] const vk::raii::ImageView& GetImageView() const;

    private:
        Texture::Ref texture;
        vk::raii::ImageView imageView;

    public:
        class Builder {
        public:
            explicit Builder(const Device& device, Texture::Ref texture);

            Builder& SetLabel(const std::string& label);
            Builder& SetType(vk::ImageViewType type);
            Builder& SetAspect(vk::ImageAspectFlags aspect);
            Builder& SetMipLevel(uint32_t base, uint32_t count = 1);
            Builder& SetArrayLayer(uint32_t base, uint32_t count = 1);

            [[nodiscard]] Ref Build();

        private:
            const Device& device;
            Texture::Ref texture;
            vk::ImageViewCreateInfo imageViewInfo;
#if defined(DVK_ENABLE_DEBUG_LABELS)
            std::string label;
#endif
        };
    };
}
