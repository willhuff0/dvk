#include <dvk/BindPool.hpp>
#include "dvk/Device.hpp"

#include <utility>
#include <algorithm>

using namespace dvk;

// BindPool
BindPool::Builder BindPool::Create(const Device& device) {
    return Builder(device);
}

BindPool::BindPool(vk::raii::DescriptorPool descriptorPool) : descriptorPool(std::move(descriptorPool)) { }

const vk::raii::DescriptorPool& BindPool::GetDescriptorPool() const {
    return descriptorPool;
}

// BindPool Builder
BindPool::Builder::Builder(const Device& device) : device(device) { }

BindPool::Builder& BindPool::Builder::SetLabel(const std::string& label_) {
#if defined(DVK_ENABLE_DEBUG_LABELS)
    label = label_;
#endif
    return *this;
}

BindPool::Builder& BindPool::Builder::AddEntry(vk::DescriptorType type, uint32_t count) {
    poolSizes.emplace_back(vk::DescriptorPoolSize {
        .type = type,
        .descriptorCount = count,
    });
    return *this;
}

BindPool::Builder& BindPool::Builder::SetMaxSets(uint32_t count) {
    maxSets = count;
    return *this;
}

BindPool::Builder& BindPool::Builder::SetFlags(vk::DescriptorPoolCreateFlags flags) {
    this->flags = flags;
    return *this;
}

BindPool::Ref BindPool::Builder::Build() {
    auto descriptorPool = vk::raii::DescriptorPool(device.GetDevice(), {
        .flags = flags | vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = maxSets,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data(),
    });

#if defined(DVK_ENABLE_DEBUG_LABELS)
    if (!label.empty()) device.GetDevice().setDebugUtilsObjectNameEXT(*descriptorPool, label);
#endif

    return std::make_shared<BindPool>(std::move(descriptorPool));
}
