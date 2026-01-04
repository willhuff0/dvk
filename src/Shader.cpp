#include <dvk/Shader.hpp>
#include "dvk/Device.hpp"

#include <dvk/Config.hpp>

using namespace dvk;

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
