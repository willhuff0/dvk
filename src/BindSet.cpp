#include <dvk/BindSet.hpp>

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

TextureBinder Binder::BindTexture(uint32_t binding) {
    return TextureBinder(*this, binding);
}

SamplerBinder Binder::BindSampler(uint32_t binding) {
    return SamplerBinder(*this, binding);
}

void Binder::Update(const vk::raii::DescriptorSet& descriptorSet) {
    std::vector<vk::WriteDescriptorSet> writesVector;
    writesVector.reserve(writes.size());

    for (auto& [binding, write] : writes) {
        write.dstSet = descriptorSet;
        switch (write.descriptorType) {
            case vk::DescriptorType::eUniformBuffer:
            case vk::DescriptorType::eStorageBuffer:
            case vk::DescriptorType::eUniformBufferDynamic:
            case vk::DescriptorType::eStorageBufferDynamic:
                bufferInfos.at(binding).buffer = buffers.at(binding)->GetBuffer();
                write.pBufferInfo = &bufferInfos.at(binding);
                break;
            case vk::DescriptorType::eSampledImage:
            case vk::DescriptorType::eStorageImage:
                imageInfos.at(binding).imageView = textureViews.at(binding)->GetImageView();
                write.pImageInfo = &imageInfos.at(binding);
                break;
            case vk::DescriptorType::eSampler:
                imageInfos.at(binding).sampler = samplers.at(binding)->GetSampler();
                write.pImageInfo = &imageInfos.at(binding);
                break;
            case vk::DescriptorType::eCombinedImageSampler:
                imageInfos.at(binding).imageView = textureViews.at(binding)->GetImageView();
                imageInfos.at(binding).sampler = samplers.at(binding)->GetSampler();
                write.pImageInfo = &imageInfos.at(binding);
                break;
            default:
                break;
        }
        writesVector.emplace_back(write);
    }

    descriptorSet.getDevice().updateDescriptorSets(writesVector, {});
}

void Binder::Update(const BindSet::Ref& bindSet) {
    Update(bindSet->GetDescriptorSet());
}

void Binder::addBufferWrite(vk::WriteDescriptorSet writeInfo, vk::DescriptorBufferInfo bufferInfo, Buffer::Ref buffer) {
    writes[writeInfo.dstBinding] = std::move(writeInfo);
    bufferInfos[writeInfo.dstBinding] = std::move(bufferInfo);
    buffers[writeInfo.dstBinding] = std::move(buffer);
}

void Binder::addImageWrite(vk::WriteDescriptorSet writeInfo, vk::DescriptorImageInfo imageInfo, TextureView::Ref textureView, Sampler::Ref sampler) {
    writes[writeInfo.dstBinding] = std::move(writeInfo);
    imageInfos[writeInfo.dstBinding] = std::move(imageInfo);
    if (textureView != nullptr) textureViews[writeInfo.dstBinding] = std::move(textureView);
    if (sampler != nullptr) samplers[writeInfo.dstBinding] = std::move(sampler);
}

// BindSet BufferBinder
BufferBinder::BufferBinder(Binder& binder, uint32_t binding) : binder(binder) {
    writeInfo.dstBinding = binding;
    writeInfo.dstArrayElement = 0;
    writeInfo.descriptorCount = 1;
    writeInfo.descriptorType = vk::DescriptorType::eUniformBuffer;
    bufferInfo.range = vk::WholeSize;
}

BufferBinder& BufferBinder::SetBufferType(vk::DescriptorType type) {
    writeInfo.descriptorType = type;
    return *this;
}

BufferBinder& BufferBinder::SetBuffer(Buffer::Ref buffer) {
    this->buffer = std::move(buffer);
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
    binder.addBufferWrite(writeInfo, bufferInfo, std::move(buffer));
    return binder;
}

// BindSet ImageBinder
TextureBinder::TextureBinder(Binder& binder, uint32_t binding) : binder(binder) {
    writeInfo.dstBinding = binding;
    writeInfo.dstArrayElement = 0;
    writeInfo.descriptorCount = 1;
    writeInfo.descriptorType = vk::DescriptorType::eSampledImage;

    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
}

TextureBinder& TextureBinder::SetTextureType(vk::DescriptorType type) {
    writeInfo.descriptorType = type;
    return *this;
}

TextureBinder& TextureBinder::SetTextureView(TextureView::Ref textureView) {
    this->textureView = std::move(textureView);
    return *this;
}

TextureBinder& TextureBinder::SetImageLayout(vk::ImageLayout layout) {
    imageInfo.imageLayout = layout;
    return *this;
}

TextureBinder& TextureBinder::SetCombinedSampler(Sampler::Ref sampler) {
    this->sampler = std::move(sampler);
    return *this;
}

TextureBinder& TextureBinder::SetArrayTarget(uint32_t index, uint32_t count) {
    writeInfo.dstArrayElement = index;
    writeInfo.descriptorCount = count;
    return *this;
}

Binder& TextureBinder::FinishTexture() const {
    binder.addImageWrite(writeInfo, imageInfo, std::move(textureView), std::move(sampler));
    return binder;
}

// BindSet SamplerBinder
SamplerBinder::SamplerBinder(Binder& binder, uint32_t binding) : binder(binder) {
    writeInfo.dstBinding = binding;
    writeInfo.dstArrayElement = 0;
    writeInfo.descriptorCount = 1;
    writeInfo.descriptorType = vk::DescriptorType::eSampler;
}

SamplerBinder& SamplerBinder::SetSampler(Sampler::Ref sampler) {
    this->sampler = std::move(sampler);
    return *this;
}

SamplerBinder& SamplerBinder::SetArrayTarget(uint32_t index, uint32_t count) {
    writeInfo.dstArrayElement = index;
    writeInfo.descriptorCount = count;
    return *this;
}

Binder& SamplerBinder::FinishSampler() const {
    binder.addImageWrite(writeInfo, imageInfo, nullptr, std::move(sampler));
    return binder;
}
