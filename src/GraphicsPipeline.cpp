#include <dvk/GraphicsPipeline.hpp>
#include "dvk/Device.hpp"

#include <utility>

#include "dvk/Shader.hpp"

using namespace dvk;

// Graphics Pipeline
GraphicsPipeline::Builder GraphicsPipeline::Create(const Device& device, PipelineLayout::Ref layout) {
    return Builder(device, std::move(layout));
}
GraphicsPipeline::GraphicsPipeline(PipelineLayout::Ref layout, vk::raii::Pipeline pipeline)
    : layout(std::move(layout)), pipeline(std::move(pipeline)) { }

const PipelineLayout::Ref& GraphicsPipeline::GetLayout() const {
    return layout;
}

const vk::raii::Pipeline& GraphicsPipeline::GetPipeline() const {
    return pipeline;
}

// Graphics Pipeline Builder
GraphicsPipeline::Builder::Builder(const Device& device, PipelineLayout::Ref layout)
    : device(device), layout(std::move(layout)) {
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;

    rasterizationState.depthClampEnable = vk::False;
    rasterizationState.rasterizerDiscardEnable = vk::False;
    rasterizationState.polygonMode = vk::PolygonMode::eFill;
    rasterizationState.cullMode = vk::CullModeFlagBits::eNone;
    rasterizationState.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizationState.depthBiasEnable = vk::False;
    rasterizationState.depthBiasSlopeFactor = 0.0f;
    rasterizationState.depthBiasConstantFactor = 0.0f;
    rasterizationState.depthBiasClamp = 0.0f;
    rasterizationState.lineWidth = 1.0f;

    multisampleState.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampleState.sampleShadingEnable = vk::False;

    colorBlendState.logicOpEnable = vk::False;
    colorBlendState.logicOp = vk::LogicOp::eCopy;

    depthStencilState.depthTestEnable = vk::False;
    depthStencilState.depthWriteEnable = vk::False;
    depthStencilState.depthCompareOp = vk::CompareOp::eLessOrEqual;
    depthStencilState.depthBoundsTestEnable = vk::False;
    depthStencilState.minDepthBounds = 0.0f;
    depthStencilState.maxDepthBounds = 1.0f;
    depthStencilState.stencilTestEnable = vk::False;
}

GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetLabel(const std::string& label_) {
#if defined(DVK_ENABLE_DEBUG_LABELS)
    label = label_;
#endif
    return *this;
}

GraphicsPipeline::StageBuilder GraphicsPipeline::Builder::AddStage(vk::ShaderStageFlagBits stage, const Shader& shader, const char* entry) {
    return StageBuilder(*this, stage, shader, entry);
}

GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetVertexInput(const std::vector<vk::VertexInputBindingDescription>& bindings, const std::vector<vk::VertexInputAttributeDescription>& attributes) {
    vertexInputBindings = bindings;
    vertexInputAttributes = attributes;
    return *this;
}

GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetTopology(vk::PrimitiveTopology topology) {
    inputAssembly.topology = topology;
    return *this;
}

GraphicsPipeline::RasterizerBuilder GraphicsPipeline::Builder::SetRasterizer() {
    return RasterizerBuilder(*this);
}

GraphicsPipeline::MultisampleBuilder GraphicsPipeline::Builder::SetMultisample() {
    return MultisampleBuilder(*this);
}

GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetColorBlendLogicOp(bool enabled, vk::LogicOp logicOp) {
    colorBlendState.logicOpEnable = enabled ? vk::True : vk::False;
    colorBlendState.logicOp = logicOp;
    return *this;
}

GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetColorBlendConstant(float r, float g, float b, float a) {
    colorBlendState.blendConstants[0] = r;
    colorBlendState.blendConstants[1] = g;
    colorBlendState.blendConstants[2] = b;
    colorBlendState.blendConstants[3] = a;
    return *this;
}

GraphicsPipeline::AttachmentBuilder GraphicsPipeline::Builder::AddAttachment() {
    return AttachmentBuilder(*this);
}

GraphicsPipeline::DepthBuilder GraphicsPipeline::Builder::SetDepth() {
    return DepthBuilder(*this);
}

GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetSingleAttachment(vk::Format format) {
    attachmentFormats.clear();
    colorBlendAttachments.clear();

    attachmentFormats.emplace_back(format);
    colorBlendAttachments.emplace_back(vk::PipelineColorBlendAttachmentState {
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    });

    return *this;
}

GraphicsPipeline::Builder& GraphicsPipeline::Builder::EnableMultisample(vk::SampleCountFlagBits samples) {
    multisampleState.rasterizationSamples = samples;
    return *this;
}

GraphicsPipeline::Builder& GraphicsPipeline::Builder::EnableDepth(vk::Format format) {
    depthStencilState.depthTestEnable = vk::True;
    depthStencilState.depthWriteEnable = vk::True;
    depthFormat = format;
    return *this;
}

void GraphicsPipeline::Builder::addStage(vk::PipelineShaderStageCreateInfo stageInfo, std::unordered_map<uint32_t, std::pair<std::unique_ptr<unsigned char[]>, size_t>> specialization, size_t totalSpecializationMapEntries, size_t specializationDataSize) {
    shaderStages.emplace_back(stageInfo);
    this->specializationDataSize += specializationDataSize;
    this->totalSpecializationMapEntries += totalSpecializationMapEntries;
    if (!specialization.empty()) {
        shaderStageSpecializations.emplace(shaderStages.size() - 1, std::move(specialization));
    }
}

void GraphicsPipeline::Builder::addAttachment(vk::Format format, vk::PipelineColorBlendAttachmentState colorBlendAttachment) {
    attachmentFormats.emplace_back(format);
    colorBlendAttachments.emplace_back(colorBlendAttachment);
}

GraphicsPipeline::Ref GraphicsPipeline::Builder::Build() {
    vk::PipelineVertexInputStateCreateInfo vertexInputState {
        .vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size()),
        .pVertexBindingDescriptions = vertexInputBindings.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size()),
        .pVertexAttributeDescriptions = vertexInputAttributes.data(),
    };

    vk::PipelineViewportStateCreateInfo viewportState {
        .viewportCount = 1,
        .scissorCount = 1,
    };

    static std::vector dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };
    vk::PipelineDynamicStateCreateInfo dynamicState {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data(),
    };

    auto specializationData = std::make_unique<unsigned char[]>(specializationDataSize);
    std::vector<vk::SpecializationInfo> specializationInfos(shaderStageSpecializations.size());
    std::vector<vk::SpecializationMapEntry> specializationMapEntries(totalSpecializationMapEntries);
    size_t specializationInfoIndex = 0;
    size_t specializationMapEntryIndex = 0;
    size_t specializationDataOffset = 0;
    for (auto& [shaderStageIndex, constants] : shaderStageSpecializations) {
        auto& specializationInfo = specializationInfos[specializationInfoIndex];

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

        shaderStages[shaderStageIndex].pSpecializationInfo = &specializationInfos[specializationInfoIndex];

        specializationInfoIndex++;
    }

    colorBlendState.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
    colorBlendState.pAttachments = colorBlendAttachments.data();

    vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
        vk::GraphicsPipelineCreateInfo {
            .stageCount = static_cast<uint32_t>(shaderStages.size()),
            .pStages = shaderStages.data(),
            .pVertexInputState = &vertexInputState,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizationState,
            .pMultisampleState = &multisampleState,
            .pDepthStencilState = &depthStencilState,
            .pColorBlendState = &colorBlendState,
            .pDynamicState = &dynamicState,
            .layout = layout->GetPipelineLayout(),
            .renderPass = nullptr,
        },
        vk::PipelineRenderingCreateInfo {
            .colorAttachmentCount = static_cast<uint32_t>(attachmentFormats.size()),
            .pColorAttachmentFormats = attachmentFormats.data(),
            .depthAttachmentFormat = depthFormat,
        },
    };

    auto pipeline = vk::raii::Pipeline(device.GetDevice(), nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());

#if defined(DVK_ENABLE_DEBUG_LABELS)
    if (!label.empty()) device.GetDevice().setDebugUtilsObjectNameEXT(*pipeline, label);
#endif

    return std::make_shared<GraphicsPipeline>(layout, std::move(pipeline));
}

// Stage Builder
GraphicsPipeline::StageBuilder::StageBuilder(Builder& builder, vk::ShaderStageFlagBits stage, const Shader& shader, const char* entry) : builder(builder) {
    stageInfo.stage = stage;
    stageInfo.module = shader.GetShaderModule();
    stageInfo.pName = entry;
}

GraphicsPipeline::StageBuilder& GraphicsPipeline::StageBuilder::SetConstant(uint32_t id, void* src, size_t size) {
    auto ptr = std::make_unique<unsigned char[]>(size);
    std::memcpy(ptr.get(), src, size);
    if (specialization.contains(id)) {
        specializationDataSize -= specialization[id].second;
    } else {
        totalSpecializationMapEntries++;
    }
    specialization[id] = std::make_pair(std::move(ptr), size);
    specializationDataSize += size;
    return *this;
}

GraphicsPipeline::StageBuilder& GraphicsPipeline::StageBuilder::SetConstant(uint32_t id, uint32_t value) {
    SetConstant(id, &value, sizeof(uint32_t));
    return *this;
}

GraphicsPipeline::Builder& GraphicsPipeline::StageBuilder::FinishStage() {
    builder.addStage(stageInfo, std::move(specialization), totalSpecializationMapEntries, specializationDataSize);
    return builder;
}

// Rasterizer Builder
GraphicsPipeline::RasterizerBuilder::RasterizerBuilder(Builder& builder) : builder(builder) { }

GraphicsPipeline::RasterizerBuilder& GraphicsPipeline::RasterizerBuilder::EnableDepthClamp(bool enabled) {
    builder.rasterizationState.depthBiasClamp = enabled ? vk::True : vk::False;
    return *this;
}

GraphicsPipeline::RasterizerBuilder& GraphicsPipeline::RasterizerBuilder::SetPolygonMode(vk::PolygonMode polygonMode) {
    builder.rasterizationState.polygonMode = polygonMode;
    return *this;
}

GraphicsPipeline::RasterizerBuilder& GraphicsPipeline::RasterizerBuilder::SetCullMode(vk::CullModeFlags cullMode) {
    builder.rasterizationState.cullMode = cullMode;
    return *this;
}

GraphicsPipeline::RasterizerBuilder& GraphicsPipeline::RasterizerBuilder::SetFrontFace(vk::FrontFace frontFace) {
    builder.rasterizationState.frontFace = frontFace;
    return *this;
}

GraphicsPipeline::RasterizerBuilder& GraphicsPipeline::RasterizerBuilder::SetDepthBias(bool enabled, float constant, float slope, float clamp) {
    builder.rasterizationState.depthBiasEnable = enabled ? vk::True : vk::False;
    builder.rasterizationState.depthBiasConstantFactor = constant;
    builder.rasterizationState.depthBiasSlopeFactor = slope;
    builder.rasterizationState.depthBiasClamp = clamp;
    return *this;
}

GraphicsPipeline::RasterizerBuilder& GraphicsPipeline::RasterizerBuilder::SetLineWidth(float lineWidth) {
    builder.rasterizationState.lineWidth = lineWidth;
    return *this;
}

GraphicsPipeline::Builder& GraphicsPipeline::RasterizerBuilder::FinishRasterizer() const {
    return builder;
}

// Multisample Builder
GraphicsPipeline::MultisampleBuilder::MultisampleBuilder(Builder& builder) : builder(builder) { }

GraphicsPipeline::MultisampleBuilder& GraphicsPipeline::MultisampleBuilder::SetSampleCount(vk::SampleCountFlagBits samples) {
    builder.multisampleState.rasterizationSamples = samples;
    return *this;
}

GraphicsPipeline::MultisampleBuilder& GraphicsPipeline::MultisampleBuilder::SetAlphaMode(bool alphaToCoverage, bool alphaToOne) {
    builder.multisampleState.alphaToCoverageEnable = alphaToCoverage;
    builder.multisampleState.alphaToOneEnable = alphaToOne;
    return *this;
}

GraphicsPipeline::MultisampleBuilder& GraphicsPipeline::MultisampleBuilder::SetSampleShading(bool enabled, float min) {
    builder.multisampleState.sampleShadingEnable = enabled ? vk::True : vk::False;
    builder.multisampleState.minSampleShading = min;
    return *this;
}

GraphicsPipeline::Builder& GraphicsPipeline::MultisampleBuilder::FinishMultisample() const {
    return builder;
}

// Attachment Builder
GraphicsPipeline::AttachmentBuilder::AttachmentBuilder(Builder& builder) : builder(builder) {
    colorBlendAttachment.blendEnable = vk::False;
    colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
}

GraphicsPipeline::AttachmentBuilder& GraphicsPipeline::AttachmentBuilder::SetFormat(vk::Format format) {
    this->format = format;
    return *this;
}

GraphicsPipeline::AttachmentBuilder& GraphicsPipeline::AttachmentBuilder::EnableBlending(bool enabled) {
    colorBlendAttachment.blendEnable = enabled ? vk::True : vk::False;
    return *this;
}

GraphicsPipeline::AttachmentBuilder& GraphicsPipeline::AttachmentBuilder::SetColorBlend(vk::BlendOp op, vk::BlendFactor src, vk::BlendFactor dst) {
    colorBlendAttachment.colorBlendOp = op;
    colorBlendAttachment.srcColorBlendFactor = src;
    colorBlendAttachment.dstColorBlendFactor = dst;
    return *this;
}

GraphicsPipeline::AttachmentBuilder& GraphicsPipeline::AttachmentBuilder::SetAlphaBlend(vk::BlendOp op, vk::BlendFactor src, vk::BlendFactor dst) {
    colorBlendAttachment.alphaBlendOp = op;
    colorBlendAttachment.srcAlphaBlendFactor = src;
    colorBlendAttachment.dstAlphaBlendFactor = dst;
    return *this;
}

GraphicsPipeline::AttachmentBuilder& GraphicsPipeline::AttachmentBuilder::SetColorWriteMask(vk::ColorComponentFlags writeMask) {
    colorBlendAttachment.colorWriteMask = writeMask;
    return *this;
}

GraphicsPipeline::Builder& GraphicsPipeline::AttachmentBuilder::FinishAttachment() const {
    builder.addAttachment(format, colorBlendAttachment);
    return builder;
}

// Depth Builder

GraphicsPipeline::DepthBuilder::DepthBuilder(Builder& builder) : builder(builder) { }

GraphicsPipeline::DepthBuilder& GraphicsPipeline::DepthBuilder::EnableDepthTest(bool enabled) {
    builder.depthStencilState.depthTestEnable = enabled ? vk::True : vk::False;
    return *this;
}

GraphicsPipeline::DepthBuilder& GraphicsPipeline::DepthBuilder::EnableDepthWrite(bool enabled) {
    builder.depthStencilState.depthWriteEnable = enabled ? vk::True : vk::False;
    return *this;
}

GraphicsPipeline::DepthBuilder& GraphicsPipeline::DepthBuilder::SetDepthCompare(vk::CompareOp compareOp) {
    builder.depthStencilState.depthCompareOp = compareOp;
    return *this;
}

GraphicsPipeline::DepthBuilder& GraphicsPipeline::DepthBuilder::EnableDepthBoundsTest(bool enabled) {
    builder.depthStencilState.depthBoundsTestEnable = enabled ? vk::True : vk::False;
    return *this;
}

GraphicsPipeline::DepthBuilder& GraphicsPipeline::DepthBuilder::SetDepthBounds(float min, float max) {
    builder.depthStencilState.minDepthBounds = min;
    builder.depthStencilState.maxDepthBounds = max;
    return *this;
}

GraphicsPipeline::Builder& GraphicsPipeline::DepthBuilder::FinishDepth() const {
    return builder;
}
