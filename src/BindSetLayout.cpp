#include <dvk/BindSetLayout.hpp>
#include "dvk/Device.hpp"

#include <utility>

using namespace dvk;

// BindSetLayout
BindSetLayout::Builder BindSetLayout::Create(const Device& device) {
    return Builder(device);
}
BindSetLayout::BindSetLayout(vk::raii::DescriptorSetLayout descriptorSetLayout)
    : descriptorSetLayout(std::move(descriptorSetLayout)) { }

const vk::raii::DescriptorSetLayout& BindSetLayout::GetDescriptorSetLayout() const {
    return descriptorSetLayout;
}

// Layout Builder
BindSetLayout::Builder::Builder(const Device& device) : device(device) { }

BindSetLayout::Builder& BindSetLayout::Builder::SetLabel(const std::string& label_) {
#if defined(DVK_ENABLE_DEBUG_LABELS)
    label = label_;
#endif
    return *this;
}

BindSetLayout::EntryBuilder BindSetLayout::Builder::AddEntry(uint32_t binding) {
    return EntryBuilder(*this, binding);
}

BindSetLayout::Ref BindSetLayout::Builder::Build() {
    auto descriptorSetLayout = vk::raii::DescriptorSetLayout(device.GetDevice(), {
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    });

#if defined(DVK_ENABLE_DEBUG_LABELS)
    if (!label.empty()) device.GetDevice().setDebugUtilsObjectNameEXT(*descriptorSetLayout, label);
#endif

    return std::make_shared<BindSetLayout>(std::move(descriptorSetLayout));
}

void BindSetLayout::Builder::addBinding(vk::DescriptorSetLayoutBinding binding) {
    bindings.emplace_back(binding);
}

// Entry Builder
BindSetLayout::EntryBuilder::EntryBuilder(Builder& builder, uint32_t binding) : builder(builder) {
    this->binding.binding = binding;
    this->binding.descriptorCount = 1;
}

BindSetLayout::EntryBuilder& BindSetLayout::EntryBuilder::SetType(vk::DescriptorType type) {
    binding.descriptorType = type;
    return *this;
}

BindSetLayout::EntryBuilder& BindSetLayout::EntryBuilder::SetCount(uint32_t count) {
    binding.descriptorCount = count;
    return *this;
}

BindSetLayout::EntryBuilder& BindSetLayout::EntryBuilder::SetStage(vk::ShaderStageFlags stage) {
    binding.stageFlags = stage;
    return *this;
}

BindSetLayout::Builder& BindSetLayout::EntryBuilder::FinishEntry() const {
    builder.addBinding(binding);
    return builder;
}
