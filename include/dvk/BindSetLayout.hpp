#pragma once

#include <dvk/Config.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <memory>
#include <vector>
#include <string>

namespace dvk {
    class Device;

    class BindSetLayout {
    public:
        class Builder;
        class EntryBuilder;
        using Ref = std::shared_ptr<BindSetLayout>;

        static Builder Create(const Device& device);
        explicit BindSetLayout(vk::raii::DescriptorSetLayout descriptorSetLayout);

        [[nodiscard]] const vk::raii::DescriptorSetLayout& GetDescriptorSetLayout() const;

    private:
        vk::raii::DescriptorSetLayout descriptorSetLayout;

    public:
        class Builder {
            friend class EntryBuilder;
        public:
            explicit Builder(const Device& device);

            Builder& SetLabel(const std::string& label);
            EntryBuilder AddEntry(uint32_t binding);

            [[nodiscard]] Ref Build();

        private:
            void addBinding(vk::DescriptorSetLayoutBinding binding);

            const Device& device;
            std::vector<vk::DescriptorSetLayoutBinding> bindings;
#if defined(DVK_ENABLE_DEBUG_LABELS)
            std::string label;
#endif
        };

        class EntryBuilder {
        public:
            EntryBuilder(Builder& builder, uint32_t binding);

            EntryBuilder& SetType(vk::DescriptorType type);
            EntryBuilder& SetCount(uint32_t count);
            EntryBuilder& SetStage(vk::ShaderStageFlags stage);

            Builder& FinishEntry() const;

        private:
            Builder& builder;
            vk::DescriptorSetLayoutBinding binding;
        };
    };
}