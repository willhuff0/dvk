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

BindSetLayout::Builder& BindSetLayout::Builder::SetFlags(vk::DescriptorSetLayoutCreateFlags flags) {
    this->flags = flags;
    return *this;
}

BindSetLayout::EntryBuilder BindSetLayout::Builder::AddEntry(uint32_t binding) {
    return EntryBuilder(*this, binding);
}

BindSetLayout::Ref BindSetLayout::Builder::Build() {
    vk::StructureChain descriptorCreateInfoChain = {
        vk::DescriptorSetLayoutCreateInfo {
            .flags = flags,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data(),
        },
        vk::DescriptorSetLayoutBindingFlagsCreateInfo {
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindingFlags = bindingFlags.data(),
        },
    };

    auto descriptorSetLayout = vk::raii::DescriptorSetLayout(device.GetDevice(), descriptorCreateInfoChain.get<vk::DescriptorSetLayoutCreateInfo>());

#if defined(DVK_ENABLE_DEBUG_LABELS)
    if (!label.empty()) device.GetDevice().setDebugUtilsObjectNameEXT(*descriptorSetLayout, label);
#endif

    return std::make_shared<BindSetLayout>(std::move(descriptorSetLayout));
}

void BindSetLayout::Builder::addBinding(vk::DescriptorSetLayoutBinding binding, vk::DescriptorBindingFlags flags) {
    bindings.emplace_back(binding);
    bindingFlags.emplace_back(flags);
}

// Entry Builder
BindSetLayout::EntryBuilder::EntryBuilder(Builder& builder, uint32_t binding) : builder(builder) {
    this->binding.binding = binding;
    this->binding.descriptorCount = 1;

    this->flags = {};
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

BindSetLayout::EntryBuilder& BindSetLayout::EntryBuilder::SetFlags(vk::DescriptorBindingFlags flags) {
    this->flags = flags;
    return *this;
}

BindSetLayout::Builder& BindSetLayout::EntryBuilder::FinishEntry(uint32_t repeat) const {
    for (uint32_t i = 0; i < repeat; i++) {
        vk::DescriptorSetLayoutBinding bindingClone = binding;
        bindingClone.binding += i;
        builder.addBinding(std::move(bindingClone), flags);
    }
    return builder;
}
