#include <dvk/ComputePipeline.hpp>
#include "dvk/Device.hpp"

#include <utility>

#include "dvk/Shader.hpp"

using namespace dvk;

// ComputePipeline
ComputePipeline::Builder ComputePipeline::Create(const Device& device, PipelineLayout::Ref layout) {
    return Builder(device, std::move(layout));
}
ComputePipeline::ComputePipeline(PipelineLayout::Ref layout, vk::raii::Pipeline pipeline)
    : layout(std::move(layout)), pipeline(std::move(pipeline)) { }

const PipelineLayout::Ref& ComputePipeline::GetLayout() const {
    return layout;
}

const vk::raii::Pipeline& ComputePipeline::GetPipeline() const {
    return pipeline;
}

// ComputePipeline Builder
ComputePipeline::Builder::Builder(const Device& device, PipelineLayout::Ref layout)
    : device(device), layout(std::move(layout)) { }

ComputePipeline::Builder& ComputePipeline::Builder::SetLabel(const std::string& label_) {
#if defined(DVK_ENABLE_DEBUG_LABELS)
    label = label_;
#endif
    return *this;
}

StageBuilder<ComputePipeline::Builder> ComputePipeline::Builder::SetStage(const Shader& shader, const char* entry) {
    return StageBuilder(*this, vk::ShaderStageFlagBits::eCompute, shader, entry);
}

void ComputePipeline::Builder::addStage(vk::PipelineShaderStageCreateInfo stageInfo, std::unordered_map<uint32_t, std::pair<std::unique_ptr<unsigned char[]>, size_t>> specialization, size_t totalSpecializationMapEntries, size_t specializationDataSize) {
    shaderStage = std::move(stageInfo);
    this->specializationDataSize += specializationDataSize;
    this->totalSpecializationMapEntries += totalSpecializationMapEntries;
    shaderStageSpecialization = std::move(specialization);
}

ComputePipeline::Ref ComputePipeline::Builder::Build() {
    auto specializationData = std::make_unique<unsigned char[]>(specializationDataSize);
    vk::SpecializationInfo specializationInfo;
    std::vector<vk::SpecializationMapEntry> specializationMapEntries(totalSpecializationMapEntries);
    if (!shaderStageSpecialization.empty()) {
        size_t specializationMapEntryIndex = 0;
        size_t specializationDataOffset = 0;

        auto& constants = shaderStageSpecialization;

        specializationInfo.mapEntryCount = constants.size();
        specializationInfo.pMapEntries = &specializationMapEntries[specializationMapEntryIndex];
        specializationInfo.dataSize = specializationDataSize;
        specializationInfo.pData = specializationData.get();

        for (auto& [constantId, data] : constants) {
            auto& mapEntry = specializationMapEntries[specializationMapEntryIndex];

            mapEntry.constantID = constantId;
            mapEntry.offset = specializationDataOffset;
            mapEntry.size = data.second;

            std::memcpy(specializationData.get() + specializationDataOffset, data.first.get(), data.second);

            specializationMapEntryIndex++;
            specializationDataOffset += data.second;
        }

        shaderStage.pSpecializationInfo = &specializationInfo;
    }

    auto pipeline = vk::raii::Pipeline(device.GetDevice(), nullptr, vk::ComputePipelineCreateInfo {
        .stage = shaderStage,
        .layout = layout->GetPipelineLayout(),
    });

#if defined(DVK_ENABLE_DEBUG_LABELS)
    if (!label.empty()) device.GetDevice().setDebugUtilsObjectNameEXT(*pipeline, label);
#endif

    return std::make_shared<ComputePipeline>(layout, std::move(pipeline));
}
