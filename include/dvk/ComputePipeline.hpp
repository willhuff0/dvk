#pragma once

#include <dvk/Config.hpp>
#include "PipelineLayout.hpp"
#include "PipelineCommon.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace dvk {
    class Device;
    class Shader;

    class ComputePipeline {
    public:
        class Builder;
        using Ref = std::shared_ptr<ComputePipeline>;

        static Builder Create(const Device& device, PipelineLayout::Ref layout);
        ComputePipeline(PipelineLayout::Ref layout, vk::raii::Pipeline pipeline);

        [[nodiscard]] const PipelineLayout::Ref& GetLayout() const;
        [[nodiscard]] const vk::raii::Pipeline& GetPipeline() const;

    private:
        PipelineLayout::Ref layout;
        vk::raii::Pipeline pipeline;

    public:
        class Builder {
            friend class StageBuilder<Builder>;
        public:
            Builder(const Device& device, PipelineLayout::Ref layout);

            Builder& SetLabel(const std::string& label);
            StageBuilder<Builder> SetStage(const Shader& shader, const char* entry);

            [[nodiscard]] Ref Build();

        private:
            void addStage(vk::PipelineShaderStageCreateInfo stageInfo, std::unordered_map<uint32_t, std::pair<std::unique_ptr<unsigned char[]>, size_t>> specialization, size_t totalSpecializationMapEntries, size_t specializationDataSize);

            const Device& device;
            PipelineLayout::Ref layout;
            vk::PipelineShaderStageCreateInfo shaderStage;
            std::unordered_map<uint32_t, std::pair<std::unique_ptr<unsigned char[]>, size_t>> shaderStageSpecialization;
            size_t totalSpecializationMapEntries = 0;
            size_t specializationDataSize = 0;
#if defined(DVK_ENABLE_DEBUG_LABELS)
            std::string label;
#endif
        };
    };
}