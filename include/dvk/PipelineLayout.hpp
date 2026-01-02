#pragma once

#include <dvk/Config.hpp>
#include "BindSetLayout.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <memory>
#include <vector>
#include <string>

namespace dvk {
    class Device;

    class PipelineLayout {
    public:
        class Builder;
        class PushConstantRangeBuilder;
        using Ref = std::shared_ptr<PipelineLayout>;

        static Builder Create(const Device& device);
        explicit PipelineLayout(vk::raii::PipelineLayout pipelineLayout);

        [[nodiscard]] const vk::raii::PipelineLayout& GetPipelineLayout() const;

    private:
        vk::raii::PipelineLayout pipelineLayout;

    public:
        class Builder {
            friend class PushConstantRangeBuilder;
        public:
            explicit Builder(const Device& device);

            Builder& SetLabel(const std::string& label);
            Builder& AddBindSetLayout(BindSetLayout::Ref bindSetLayout);
            PushConstantRangeBuilder AddPushConstantRange();

            [[nodiscard]] Ref Build();

        private:
            void addPushConstantRange(vk::PushConstantRange pushConstantRange);

            const Device& device;
            std::vector<BindSetLayout::Ref> bindSetLayouts;
            std::vector<vk::PushConstantRange> pushConstantRanges;
#if defined(DVK_ENABLE_DEBUG_LABELS)
            std::string label;
#endif
        };

        class PushConstantRangeBuilder {
        public:
            explicit PushConstantRangeBuilder(Builder& builder);

            PushConstantRangeBuilder& SetStage(vk::ShaderStageFlags stage);
            PushConstantRangeBuilder& SetOffset(uint32_t offset);
            PushConstantRangeBuilder& SetSize(uint32_t size);

            Builder& FinishPushConstantRange() const;

        private:
            Builder& builder;
            vk::PushConstantRange pushConstantRange;
        };
    };
}
