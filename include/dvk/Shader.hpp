#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <vector>
#include <string>

namespace dvk {
    class Device;

    class Shader {
    public:
        Shader(const Device& device, const std::vector<char>& code, const std::string& label = "");

        [[nodiscard]] const vk::raii::ShaderModule& GetShaderModule() const;

    private:
        vk::raii::ShaderModule shaderModule = nullptr;
    };
}