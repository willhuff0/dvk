#include <dvk/Shader.hpp>
#include "dvk/Device.hpp"

#include <dvk/Config.hpp>

#include <filesystem>
#include <fstream>

using namespace dvk;

static std::vector<char> readFile(const std::string& path) {
    if (!std::filesystem::exists(path)) throw std::runtime_error("File doesn't exist!");
    auto fileSize = std::filesystem::file_size(path);
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("Failed to open file!");
    std::vector<char> buffer(fileSize);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    file.close();
    return buffer;
}

Shader Shader::LoadFile(const Device& device, const std::string& path, const std::string& label) {
    return Shader(device, readFile(path), label);
}
Shader::Shader(const Device& device, const std::vector<char>& code, const std::string& label) {
    shaderModule = vk::raii::ShaderModule(device.GetDevice(), {
        .codeSize = code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t*>(code.data()),
    });

#if defined(DVK_ENABLE_DEBUG_LABELS)
    if (!label.empty()) device.GetDevice().setDebugUtilsObjectNameEXT(*shaderModule, label);
#endif
}

const vk::raii::ShaderModule& Shader::GetShaderModule() const {
    return shaderModule;
}
