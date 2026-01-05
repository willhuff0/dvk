#include <dvk/BindSet.hpp>
#include "dvk/Device.hpp"
#include "dvk/Buffer.hpp"

#include <dvk/Config.hpp>

#include <utility>

using namespace dvk;

// BindSet
BindSet::Ref BindSet::Create(const Device& device, const BindPool::Ref& pool, const BindSetLayout::Ref& layout, const std::string& label) {
    auto descriptorSet = std::move(vk::raii::DescriptorSets(device.GetDevice(), {
        .descriptorPool = pool->GetDescriptorPool(),
        .descriptorSetCount = 1,
        .pSetLayouts = &*layout->GetDescriptorSetLayout(),
    }).front());

#if defined(DVK_ENABLE_DEBUG_LABELS)
    if (!label.empty()) device.GetDevice().setDebugUtilsObjectNameEXT(*descriptorSet, label);
#endif

    return std::make_shared<BindSet>(std::move(descriptorSet));
}

BindSet::BindSet(vk::raii::DescriptorSet descriptorSet) : descriptorSet(std::move(descriptorSet)) { }

const vk::raii::DescriptorSet& BindSet::GetDescriptorSet() const {
    return descriptorSet;
}

// BindSet Binder
BufferBinder Binder::BindBuffer(uint32_t binding) {
    return BufferBinder(*this, binding);
}

ImageBinder Binder::BindImage(uint32_t binding) {
    return ImageBinder(*this, binding);
}

SamplerBinder Binder::BindSampler(uint32_t binding) {
    return SamplerBinder(*this, binding);
}

void Binder::Update(const vk::raii::DescriptorSet& descriptorSet) {
    for (size_t i = 0; i < writes.size(); i++) {
        writes[i].dstSet = descriptorSet;
        switch (writes[i].descriptorType) {
            case vk::DescriptorType::eUniformBuffer:
            case vk::DescriptorType::eStorageBuffer:
            case vk::DescriptorType::eUniformBufferDynamic:
            case vk::DescriptorType::eStorageBufferDynamic:
                writes[i].pBufferInfo = &bufferInfos[writeIndexToInfoIndex[i]];
                break;
            case vk::DescriptorType::eSampledImage:
            case vk::DescriptorType::eStorageImage:
            case vk::DescriptorType::eCombinedImageSampler:
            case vk::DescriptorType::eSampler:
                writes[i].pImageInfo = &imageInfos[writeIndexToInfoIndex[i]];
                break;
            default:
                break;
        }
    }

    descriptorSet.getDevice().updateDescriptorSets(writes, {});
}

void Binder::Update(const BindSet::Ref& bindSet) {
    Update(bindSet->GetDescriptorSet());
}

void Binder::addBufferWrite(vk::WriteDescriptorSet writeInfo, vk::DescriptorBufferInfo bufferInfo) {
    writes.emplace_back(writeInfo);
    bufferInfos.emplace_back(bufferInfo);
    writeIndexToInfoIndex.emplace_back(bufferInfos.size() - 1);
}

void Binder::addImageWrite(vk::WriteDescriptorSet writeInfo, vk::DescriptorImageInfo imageInfo) {
    writes.emplace_back(writeInfo);
    imageInfos.emplace_back(imageInfo);
    writeIndexToInfoIndex.emplace_back(imageInfos.size() - 1);
}

// BindSet BufferBinder
BufferBinder::BufferBinder(Binder& binder, uint32_t binding) : binder(binder) {
    writeInfo.dstBinding = binding;
    writeInfo.dstArrayElement = 0;
    writeInfo.descriptorCount = 1;
    writeInfo.descriptorType = vk::DescriptorType::eUniformBuffer;
}

BufferBinder& BufferBinder::SetBufferType(vk::DescriptorType type) {
    writeInfo.descriptorType = type;
    return *this;
}

BufferBinder& BufferBinder::SetBuffer(const vk::raii::Buffer& buffer) {
    bufferInfo.buffer = buffer;
    return *this;
}

BufferBinder& BufferBinder::SetRange(vk::DeviceSize offset, vk::DeviceSize size) {
    bufferInfo.offset = offset;
    bufferInfo.range = size;
    return *this;
}

BufferBinder& BufferBinder::SetArrayTarget(uint32_t index, uint32_t count) {
    writeInfo.dstArrayElement = index;
    writeInfo.descriptorCount = count;
    return *this;
}

Binder& BufferBinder::FinishBuffer() const {
    binder.addBufferWrite(writeInfo, bufferInfo);
    return binder;
}

// BindSet ImageBinder
ImageBinder::ImageBinder(Binder& binder, uint32_t binding) : binder(binder) {
    writeInfo.dstBinding = binding;
    writeInfo.dstArrayElement = 0;
    writeInfo.descriptorCount = 1;
    writeInfo.descriptorType = vk::DescriptorType::eSampledImage;
}

ImageBinder& ImageBinder::SetImageType(vk::DescriptorType type) {
    writeInfo.descriptorType = type;
    return *this;
}

ImageBinder& ImageBinder::SetImageView(const vk::raii::ImageView& view) {
    imageInfo.imageView = view;
    return *this;
}

ImageBinder& ImageBinder::SetImageLayout(vk::ImageLayout layout) {
    imageInfo.imageLayout = layout;
    return *this;
}

ImageBinder& ImageBinder::SetCombinedSampler(const vk::raii::Sampler& sampler) {
    imageInfo.sampler = sampler;
    return *this;
}

ImageBinder& ImageBinder::SetArrayTarget(uint32_t index, uint32_t count) {
    writeInfo.dstArrayElement = index;
    writeInfo.descriptorCount = count;
    return *this;
}

Binder& ImageBinder::FinishImage() const {
    binder.addImageWrite(writeInfo, imageInfo);
    return binder;
}

// BindSet SamplerBinder
SamplerBinder::SamplerBinder(Binder& binder, uint32_t binding) : binder(binder) {
    writeInfo.dstBinding = binding;
    writeInfo.dstArrayElement = 0;
    writeInfo.descriptorCount = 1;
    writeInfo.descriptorType = vk::DescriptorType::eSampler;
}

SamplerBinder& SamplerBinder::SetSampler(const vk::raii::Sampler& sampler) {
    imageInfo.sampler = sampler;
    return *this;
}

SamplerBinder& SamplerBinder::SetArrayTarget(uint32_t index, uint32_t count) {
    writeInfo.dstArrayElement = index;
    writeInfo.descriptorCount = count;
    return *this;
}

Binder& SamplerBinder::FinishSampler() const {
    binder.addImageWrite(writeInfo, imageInfo);
    return binder;
}
