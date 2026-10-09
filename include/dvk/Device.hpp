#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc_raii.hpp>

namespace dvk {
    constexpr uint32_t API_VERSION = vk::ApiVersion13;

    class Instance;
    class Surface;

    class Device {
    public:
        Device(const Instance& instance, const Surface& surface);
        ~Device();

        [[nodiscard]] const vk::raii::PhysicalDevice& GetPhysicalDevice() const;
        [[nodiscard]] const vk::raii::Device& GetDevice() const;
        [[nodiscard]] const vk::raii::Queue& GetQueue() const;
        [[nodiscard]] uint32_t GetQueueFamilyIndex() const;
        [[nodiscard]] const vma::raii::Allocator& GetAllocator() const;

    private:
        vk::raii::PhysicalDevice physicalDevice = nullptr;
        vk::raii::Device device = nullptr;
        vk::raii::Queue queue = nullptr;
        uint32_t queueFamilyIndex = ~0;
        vma::raii::Allocator allocator = nullptr;
    };
}