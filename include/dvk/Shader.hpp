#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <vector>
#include <string>

namespace dvk {
    class Device;

    class Shader {
    public:
        static Shader LoadFile(const Device& device, const std::string& path, const std::string& label = "");
        Shader(const Device& device, const std::vector<char>& code, const std::string& label = "");

        [[nodiscard]] const vk::raii::ShaderModule& GetShaderModule() const;

    private:
        vk::raii::ShaderModule shaderModule = nullptr;
    };
}