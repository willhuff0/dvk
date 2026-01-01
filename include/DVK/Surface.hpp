#pragma once

#include <vulkan/vulkan_raii.hpp>

namespace DVK {
    class Instance;

    class Surface {
    public:
        Surface(const Instance& instance, VkSurfaceKHR surface);

        [[nodiscard]] const vk::raii::SurfaceKHR& GetSurface() const;

    private:
        vk::raii::SurfaceKHR surface = nullptr;
    };
}