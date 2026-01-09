#pragma once

#include <dvk/Config.hpp>
#include "PipelineLayout.hpp"
#include "PipelineCommon.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <utility>

namespace dvk {
    class Device;
    class Shader;

    class GraphicsPipeline {
    public:
        class Builder;
        class RasterizerBuilder;
        class MultisampleBuilder;
        class AttachmentBuilder;
        class DepthBuilder;
        using Ref = std::shared_ptr<GraphicsPipeline>;

        static Builder Create(const Device& device, PipelineLayout::Ref layout);
        GraphicsPipeline(PipelineLayout::Ref layout, vk::raii::Pipeline pipeline);

        [[nodiscard]] const PipelineLayout::Ref& GetLayout() const;
        [[nodiscard]] const vk::raii::Pipeline& GetPipeline() const;

    private:
        PipelineLayout::Ref layout;
        vk::raii::Pipeline pipeline;

    public:
        class Builder {
            friend class StageBuilder<Builder>;
            friend class RasterizerBuilder;
            friend class MultisampleBuilder;
            friend class AttachmentBuilder;
            friend class DepthBuilder;
        public:
            Builder(const Device& device, PipelineLayout::Ref layout);

            Builder& SetLabel(const std::string& label);
            StageBuilder<Builder> AddStage(vk::ShaderStageFlagBits stage, const Shader& shader, const char* entry);
            Builder& SetVertexInput(const std::vector<vk::VertexInputBindingDescription>& bindings, const std::vector<vk::VertexInputAttributeDescription>& attributes);
            Builder& SetTopology(vk::PrimitiveTopology topology);
            RasterizerBuilder SetRasterizer();
            MultisampleBuilder SetMultisample();
            Builder& SetColorBlendLogicOp(bool enabled, vk::LogicOp logicOp);
            Builder& SetColorBlendConstant(float r, float g, float b, float a);
            AttachmentBuilder AddAttachment();
            DepthBuilder SetDepth();

            Builder& SetSingleAttachment(vk::Format format);
            Builder& EnableMultisample(vk::SampleCountFlagBits samples);
            Builder& EnableDepth(vk::Format format);

            [[nodiscard]] Ref Build();

        private:
            void addStage(vk::PipelineShaderStageCreateInfo stageInfo, std::unordered_map<uint32_t, std::pair<std::unique_ptr<unsigned char[]>, size_t>> specialization, size_t totalSpecializationMapEntries, size_t specializationDataSize);
            void addAttachment(vk::Format format, vk::PipelineColorBlendAttachmentState colorBlendAttachment);

            const Device& device;
            PipelineLayout::Ref layout;
            std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
            std::unordered_map<size_t, std::unordered_map<uint32_t, std::pair<std::unique_ptr<unsigned char[]>, size_t>>> shaderStageSpecializations;
            size_t totalSpecializationMapEntries = 0;
            size_t specializationDataSize = 0;
            std::vector<vk::VertexInputBindingDescription> vertexInputBindings;
            std::vector<vk::VertexInputAttributeDescription> vertexInputAttributes;
            vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
            vk::PipelineRasterizationStateCreateInfo rasterizationState;
            vk::PipelineMultisampleStateCreateInfo multisampleState;
            vk::PipelineColorBlendStateCreateInfo colorBlendState;
            std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments;
            std::vector<vk::Format> attachmentFormats;
            vk::PipelineDepthStencilStateCreateInfo depthStencilState;
            vk::Format depthFormat = vk::Format::eUndefined;
#if defined(DVK_ENABLE_DEBUG_LABELS)
            std::string label;
#endif
        };

        class RasterizerBuilder {
        public:
            explicit RasterizerBuilder(Builder& builder);

            RasterizerBuilder& EnableDepthClamp(bool enabled = false);
            RasterizerBuilder& SetPolygonMode(vk::PolygonMode polygonMode);
            RasterizerBuilder& SetCullMode(vk::CullModeFlags cullMode);
            RasterizerBuilder& SetFrontFace(vk::FrontFace frontFace);
            RasterizerBuilder& SetDepthBias(bool enabled, float constant = 0.0f, float slope = 1.0f, float clamp = 0.0f);
            RasterizerBuilder& SetLineWidth(float lineWidth);

            Builder& FinishRasterizer() const;

        private:
            Builder& builder;
        };

        class MultisampleBuilder {
        public:
            explicit MultisampleBuilder(Builder& builder);

            MultisampleBuilder& SetSampleCount(vk::SampleCountFlagBits samples);
            MultisampleBuilder& SetAlphaMode(bool alphaToCoverage, bool alphaToOne);
            MultisampleBuilder& SetSampleShading(bool enabled, float min = 0.1f);

            Builder& FinishMultisample() const;

        private:
            Builder& builder;
        };

        class AttachmentBuilder {
        public:
            explicit AttachmentBuilder(Builder& builder);

            AttachmentBuilder& SetFormat(vk::Format format);
            AttachmentBuilder& EnableBlending(bool enabled = true);
            AttachmentBuilder& SetColorBlend(vk::BlendOp op, vk::BlendFactor src, vk::BlendFactor dst);
            AttachmentBuilder& SetAlphaBlend(vk::BlendOp op, vk::BlendFactor src, vk::BlendFactor dst);
            AttachmentBuilder& SetColorWriteMask(vk::ColorComponentFlags writeMask);

            Builder& FinishAttachment() const;

        private:
            Builder& builder;
            vk::Format format = vk::Format::eUndefined;
            vk::PipelineColorBlendAttachmentState colorBlendAttachment;
        };

        class DepthBuilder {
        public:
            explicit DepthBuilder(Builder& builder);

            DepthBuilder& EnableDepthTest(bool enabled = true);
            DepthBuilder& EnableDepthWrite(bool enabled = true);
            DepthBuilder& SetDepthCompare(vk::CompareOp compareOp);
            DepthBuilder& EnableDepthBoundsTest(bool enabled = true);
            DepthBuilder& SetDepthBounds(float min, float max);

            Builder& FinishDepth() const;

        private:
            Builder& builder;
        };
    };
}