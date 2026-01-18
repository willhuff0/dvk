#include <dvk/Sampler.hpp>
#include "dvk/Device.hpp"

#include <utility>

using namespace dvk;

// Sampler
Sampler::Builder Sampler::Create(const Device& device) {
    return Builder(device);
}

Sampler::Sampler(vk::raii::Sampler sampler) : sampler(std::move(sampler)) { }

const vk::raii::Sampler& Sampler::GetSampler() const {
    return sampler;
}

// Sampler Builder
Sampler::Builder::Builder(const Device& device) : device(device) {
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.anisotropyEnable = vk::False;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.compareEnable = vk::False;
    samplerInfo.compareOp = vk::CompareOp::eAlways;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 32.0f;
    samplerInfo.borderColor = vk::BorderColor::eFloatTransparentBlack;
    samplerInfo.unnormalizedCoordinates = vk::False;
}

Sampler::Builder& Sampler::Builder::SetLabel(const std::string& label_) {
#if defined(DVK_ENABLE_DEBUG_LABELS)
    label = label_;
#endif
    return *this;
}

Sampler::Builder& Sampler::Builder::SetFilter(vk::Filter filter) {
    samplerInfo.minFilter = filter;
    samplerInfo.magFilter = filter;
    return *this;
}
Sampler::Builder& Sampler::Builder::SetFilter(vk::Filter min, vk::Filter mag) {
    samplerInfo.minFilter = min;
    samplerInfo.magFilter = mag;
    return *this;
}

Sampler::Builder& Sampler::Builder::SetMipMapFilter(vk::SamplerMipmapMode filter) {
    samplerInfo.mipmapMode = filter;
    return *this;
}

Sampler::Builder& Sampler::Builder::SetAddressMode(vk::SamplerAddressMode addressMode) {
    samplerInfo.addressModeU = addressMode;
    samplerInfo.addressModeV = addressMode;
    samplerInfo.addressModeW = addressMode;
    return *this;
}
Sampler::Builder& Sampler::Builder::SetAddressMode(vk::SamplerAddressMode u, vk::SamplerAddressMode v, vk::SamplerAddressMode w) {
    samplerInfo.addressModeU = u;
    samplerInfo.addressModeV = v;
    samplerInfo.addressModeW = w;
    return *this;
}

Sampler::Builder& Sampler::Builder::SetMipLodBias(float bias) {
    samplerInfo.mipLodBias = bias;
    return *this;
}

Sampler::Builder& Sampler::Builder::SetAnisotropy(bool enabled, float max) {
    samplerInfo.anisotropyEnable = enabled ? vk::True : vk::False;
    samplerInfo.maxAnisotropy = max;
    return *this;
}

Sampler::Builder& Sampler::Builder::SetCompare(bool enabled, vk::CompareOp compareOp) {
    samplerInfo.compareEnable = enabled ? vk::True : vk::False;
    samplerInfo.compareOp = compareOp;
    return *this;
}

Sampler::Builder& Sampler::Builder::SetLodBounds(float minLod, float maxLod) {
    samplerInfo.minLod = minLod;
    samplerInfo.maxLod = maxLod;
    return *this;
}

Sampler::Builder& Sampler::Builder::SetBorderColor(vk::BorderColor borderColor) {
    samplerInfo.borderColor = borderColor;
    return *this;
}

Sampler::Builder& Sampler::Builder::UseUnnormalizedCoords(bool unnormalizedCoords) {
    samplerInfo.unnormalizedCoordinates = unnormalizedCoords ? vk::True : vk::False;
    return *this;
}

Sampler::Ref Sampler::Builder::Build() const {
    auto sampler = vk::raii::Sampler(device.GetDevice(), samplerInfo);

#if defined(DVK_ENABLE_DEBUG_LABELS)
    if (!label.empty()) device.GetDevice().setDebugUtilsObjectNameEXT(*sampler, label);
#endif

    return std::make_shared<Sampler>(std::move(sampler));
}
