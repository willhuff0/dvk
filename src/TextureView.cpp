#include <dvk/TextureView.hpp>
#include "dvk/Device.hpp"

#include <utility>

using namespace dvk;

// TextureView
TextureView::Builder TextureView::Create(const Device& device, Texture::Ref texture) {
    return Builder(device, std::move(texture));
}

TextureView::TextureView(Texture::Ref texture, vk::raii::ImageView imageView)
    : texture(std::move(texture)), imageView(std::move(imageView)) { }

const Texture::Ref& TextureView::GetTexture() const {
    return texture;
}

const vk::raii::ImageView& TextureView::GetImageView() const {
    return imageView;
}

// TextureView Builder
TextureView::Builder::Builder(const Device& device, Texture::Ref texture) : device(device), texture(std::move(texture)) {
    imageViewInfo.viewType = vk::ImageViewType::e2D;
    imageViewInfo.subresourceRange = {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = vk::RemainingMipLevels,
        .baseArrayLayer = 0,
        .layerCount = vk::RemainingArrayLayers,
    };
}

TextureView::Builder& TextureView::Builder::SetLabel(const std::string& label_) {
#if defined(DVK_ENABLE_DEBUG_LABELS)
    label = label_;
#endif
    return *this;
}

TextureView::Builder& TextureView::Builder::SetType(vk::ImageViewType type) {
    imageViewInfo.viewType = type;
    return *this;
}

TextureView::Builder& TextureView::Builder::SetAspect(vk::ImageAspectFlags aspect) {
    imageViewInfo.subresourceRange.aspectMask = aspect;
    return *this;
}

TextureView::Builder& TextureView::Builder::SetMipLevel(uint32_t base, uint32_t count) {
    imageViewInfo.subresourceRange.baseMipLevel = base;
    imageViewInfo.subresourceRange.levelCount = count;
    return *this;
}

TextureView::Builder& TextureView::Builder::SetArrayLayer(uint32_t base, uint32_t count) {
    imageViewInfo.subresourceRange.baseArrayLayer = base;
    imageViewInfo.subresourceRange.layerCount = count;
    return *this;
}

TextureView::Ref TextureView::Builder::Build() {
    imageViewInfo.format = texture->GetFormat();
    auto imageView = vk::raii::ImageView(device.GetDevice(), imageViewInfo);

#if defined(DVK_ENABLE_DEBUG_LABELS)
    if (!label.empty()) device.GetDevice().setDebugUtilsObjectNameEXT(*imageView, label);
#endif

    return std::make_shared<TextureView>(texture, imageView);
}
