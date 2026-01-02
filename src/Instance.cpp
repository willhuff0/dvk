#include <dvk/Instance.hpp>

#include <ranges>
#include <string>
#include <stdexcept>

#if defined(NDEBUG)
constexpr bool ENABLE_VALIDATION_LAYERS = false;
#else
constexpr bool ENABLE_VALIDATION_LAYERS = true;
#endif

using namespace dvk;

Instance::Instance(const std::vector<const char*>& extensions, const char* appName) {
    vk::ApplicationInfo appInfo {
        .pApplicationName = appName,
        .applicationVersion = vk::makeVersion(1, 0, 0),
        .pEngineName = "DVK",
        .engineVersion = vk::makeVersion(1, 0, 0),
        .apiVersion = vk::ApiVersion13,
    };

    // Get layers
    std::vector<const char*> requiredLayers;
    if constexpr (ENABLE_VALIDATION_LAYERS) {
        requiredLayers.emplace_back("VK_LAYER_KHRONOS_validation");
    }

    auto layerProperties = context.enumerateInstanceLayerProperties();
    for (const auto& requiredLayer : requiredLayers) {
        if (std::ranges::none_of(
            layerProperties.begin(),
            layerProperties.end(),
            [&] (const auto& layerProperty) {
                return std::strcmp(requiredLayer, layerProperty.layerName) == 0;
            }
        )) {
            throw std::runtime_error("Required layer is not supported: " + std::string(requiredLayer));
        }
    }

    // Get extensions
    std::vector<const char*> requiredExtensions(extensions);
    if constexpr (ENABLE_VALIDATION_LAYERS) {
        requiredExtensions.emplace_back(vk::EXTDebugUtilsExtensionName);
    }

    auto extensionProperties = context.enumerateInstanceExtensionProperties();
    for (const auto& requiredExtension : requiredExtensions) {
        if (std::ranges::none_of(
            extensionProperties.begin(),
            extensionProperties.end(),
            [&] (const auto& extensionProperty) {
                return std::strcmp(requiredExtension, extensionProperty.extensionName) == 0;
            }
        )) {
            throw std::runtime_error("Required extension is not supported: " + std::string(requiredExtension));
        }
    }

    // Add portability extension if needed
    bool portabilityRequired = false;
    if (std::ranges::any_of(
        extensionProperties.begin(),
        extensionProperties.end(),
        [&] (const auto& extensionProperty) {
            return std::strcmp(vk::KHRPortabilityEnumerationExtensionName, extensionProperty.extensionName) == 0;
        }
    )) {
        requiredExtensions.emplace_back(vk::KHRPortabilityEnumerationExtensionName);
        portabilityRequired = true;
    }

    instance = vk::raii::Instance(context, {
        .flags = portabilityRequired ? vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR : vk::InstanceCreateFlags { },
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
        .ppEnabledLayerNames = requiredLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
        .ppEnabledExtensionNames = requiredExtensions.data(),
    });
}

const vk::raii::Instance& Instance::GetInstance() const {
    return instance;
}
