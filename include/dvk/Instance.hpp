#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <vector>

namespace dvk {
    class Instance {
    public:
        explicit Instance(const std::vector<const char*>& extensions, const char* appName = "DVK App");

        [[nodiscard]] const vk::raii::Instance& GetInstance() const;

    private:
        vk::raii::Context context;
        vk::raii::Instance instance = nullptr;
    };
}