#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace DVK {
    class Instance;
    class Surface;

    class Device {
    public:
        Device(const Instance& instance, const Surface& surface);

        [[nodiscard]] const vk::raii::PhysicalDevice& GetPhysicalDevice() const;
        [[nodiscard]] const vk::raii::Device& GetDevice() const;
        [[nodiscard]] const vk::raii::Queue& GetQueue() const;

    private:
        vk::raii::PhysicalDevice physicalDevice = nullptr;
        vk::raii::Device device = nullptr;
        vk::raii::Queue queue = nullptr;
        uint32_t queueFamily = ~0;
    };
}