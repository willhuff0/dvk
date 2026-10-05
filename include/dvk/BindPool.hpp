#pragma once

#include <dvk/Config.hpp>

#include <vulkan/vulkan_raii.hpp>

#include <memory>
#include <vector>
#include <string>

namespace dvk {
    class Device;

    class BindPool {
    public:
        class Builder;
        using Ref = std::shared_ptr<BindPool>;

        static Builder Create(const Device& device);
        explicit BindPool(vk::raii::DescriptorPool descriptorPool);

        [[nodiscard]] const vk::raii::DescriptorPool& GetDescriptorPool() const;

    private:
        vk::raii::DescriptorPool descriptorPool;

    public:
        class Builder {
        public:
            explicit Builder(const Device& device);

            Builder& SetLabel(const std::string& label);
            Builder& AddEntry(vk::DescriptorType type, uint32_t count);
            Builder& SetMaxSets(uint32_t count);
            Builder& SetFlags(vk::DescriptorPoolCreateFlags flags);

            [[nodiscard]] Ref Build();

        private:
            const Device& device;
            std::vector<vk::DescriptorPoolSize> poolSizes;
            uint32_t maxSets = 8;
            vk::DescriptorPoolCreateFlags flags;
#if defined(DVK_ENABLE_DEBUG_LABELS)
            std::string label;
#endif
        };
    };
}