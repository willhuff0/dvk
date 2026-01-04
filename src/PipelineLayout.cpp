#include <dvk/PipelineLayout.hpp>
#include "dvk/Device.hpp"

#include <utility>

using namespace dvk;

// Pipeline Layout
PipelineLayout::Builder PipelineLayout::Create(const Device& device) {
    return Builder(device);
}
PipelineLayout::PipelineLayout(vk::raii::PipelineLayout pipelineLayout)
    : pipelineLayout(std::move(pipelineLayout)) { }

const vk::raii::PipelineLayout& PipelineLayout::GetPipelineLayout() const {
    return pipelineLayout;
}

// Pipeline Layout Builder
PipelineLayout::Builder::Builder(const Device& device) : device(device) { }

PipelineLayout::Builder& PipelineLayout::Builder::SetLabel(const std::string& label_) {
#if defined(DVK_ENABLE_DEBUG_LABELS)
    label = label_;
#endif
    return *this;
}

PipelineLayout::Builder& PipelineLayout::Builder::AddBindSetLayout(BindSetLayout::Ref bindSetLayout) {
    bindSetLayouts.emplace_back(bindSetLayout);
    return *this;
}

PipelineLayout::PushConstantRangeBuilder PipelineLayout::Builder::AddPushConstantRange() {
    return PushConstantRangeBuilder(*this);
}

PipelineLayout::Ref PipelineLayout::Builder::Build() {
    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
    descriptorSetLayouts.reserve(bindSetLayouts.size());
    for (const auto& bindSetLayout : bindSetLayouts) {
        descriptorSetLayouts.emplace_back(*bindSetLayout->GetDescriptorSetLayout());
    }

    auto pipelineLayout = vk::raii::PipelineLayout(device.GetDevice(), {
        .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
        .pSetLayouts = descriptorSetLayouts.data(),
        .pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size()),
        .pPushConstantRanges = pushConstantRanges.data(),
    });

#if defined(DVK_ENABLE_DEBUG_LABELS)
    if (!label.empty()) device.GetDevice().setDebugUtilsObjectNameEXT(*pipelineLayout, label);
#endif

    return std::make_shared<PipelineLayout>(pipelineLayout);
}

void PipelineLayout::Builder::addPushConstantRange(vk::PushConstantRange pushConstantRange) {
    pushConstantRanges.emplace_back(pushConstantRange);
}

// Push Constant Range Builder
PipelineLayout::PushConstantRangeBuilder::PushConstantRangeBuilder(Builder& builder) : builder(builder) { }

PipelineLayout::PushConstantRangeBuilder& PipelineLayout::PushConstantRangeBuilder::SetStage(vk::ShaderStageFlags stage) {
    pushConstantRange.stageFlags = stage;
    return *this;
}

PipelineLayout::PushConstantRangeBuilder& PipelineLayout::PushConstantRangeBuilder::SetOffset(uint32_t offset) {
    pushConstantRange.offset = offset;
    return *this;
}

PipelineLayout::PushConstantRangeBuilder& PipelineLayout::PushConstantRangeBuilder::SetSize(uint32_t size) {
    pushConstantRange.size = size;
    return *this;
}

PipelineLayout::Builder& PipelineLayout::PushConstantRangeBuilder::FinishPushConstantRange() const {
    builder.addPushConstantRange(pushConstantRange);
    return builder;
}
