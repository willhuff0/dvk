#include <dvk/Device.hpp>
#include "dvk/Instance.hpp"
#include "dvk/Surface.hpp"

#include <algorithm>
#include <stdexcept>

using namespace dvk;

const std::vector<const char*> DEVICE_EXTENSIONS = {
    vk::KHRSwapchainExtensionName,
    vk::KHRSpirv14ExtensionName,
    vk::KHRSynchronization2ExtensionName,
    vk::KHRCreateRenderpass2ExtensionName,
};

static int getPhysicalDeviceTypeScore(vk::PhysicalDeviceType physicalDeviceType) {
    switch (physicalDeviceType) {
        case vk::PhysicalDeviceType::eOther: return 0;
        case vk::PhysicalDeviceType::eCpu: return 0;
        case vk::PhysicalDeviceType::eIntegratedGpu: return 1;
        case vk::PhysicalDeviceType::eVirtualGpu: return 1;
        case vk::PhysicalDeviceType::eDiscreteGpu: return 2;
        default: return -1;
    }
}

Device::Device(const Instance& instance, const Surface& surface) {
    // Physical Device
    {
        auto physicalDevices = instance.GetInstance().enumeratePhysicalDevices();
        std::erase_if(physicalDevices, [&](const vk::raii::PhysicalDevice& potentialPhysicalDevice) {
            auto properties = potentialPhysicalDevice.getProperties();
            if (properties.apiVersion < vk::ApiVersion13) return true;

            auto queueFamilies = potentialPhysicalDevice.getQueueFamilyProperties();
            if (std::ranges::none_of(queueFamilies, [&](const vk::QueueFamilyProperties& queueFamily) {
                return (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0);
            })) return true;
            if (std::ranges::none_of(queueFamilies, [&](const vk::QueueFamilyProperties& queueFamily) {
                return (queueFamily.queueFlags & vk::QueueFlagBits::eCompute) != static_cast<vk::QueueFlags>(0);
            })) return true;

            auto extensions = potentialPhysicalDevice.enumerateDeviceExtensionProperties();
            for (const auto& requiredExtension : DEVICE_EXTENSIONS) {
                if (std::ranges::none_of(extensions, [&](const vk::ExtensionProperties& extension) {
                    return std::strcmp(extension.extensionName, requiredExtension) == 0;
                })) return true;
            }

            auto features = potentialPhysicalDevice.getFeatures2<
                vk::PhysicalDeviceFeatures2,
                vk::PhysicalDeviceVulkan11Features,
                vk::PhysicalDeviceVulkan13Features,
                vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
            >();
            if (!features.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy) return true;
            if (!features.get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters) return true;
            if (!features.get<vk::PhysicalDeviceVulkan13Features>().synchronization2) return true;
            if (!features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering) return true;
            if (!features.get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState) return true;

            return false;
        });
        if (physicalDevices.empty()) throw std::runtime_error("Failed to find a suitable device!");

        std::ranges::sort(physicalDevices, [](const vk::raii::PhysicalDevice& a, const vk::raii::PhysicalDevice& b) {
            auto aProperties = a.getProperties();
            auto bProperties = b.getProperties();

            if (getPhysicalDeviceTypeScore(aProperties.deviceType) > getPhysicalDeviceTypeScore(bProperties.deviceType)) return true;

            return false;
        });
        physicalDevice = physicalDevices.front();
    }

    // Logical Device
    {
        std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();
        for (uint32_t i = 0; i < queueFamilies.size(); i++) {
            if (!(queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics)) continue;
            if (!(queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute)) continue;
            if (!physicalDevice.getSurfaceSupportKHR(i, *surface.GetSurface())) continue;

            queueFamilyIndex = i;
            break;
        }
        if (queueFamilyIndex == ~0) {
            throw std::runtime_error("Failed to find a queue supporting all of graphics, compute, and presentation!");
        }

        vk::StructureChain<
            vk::PhysicalDeviceFeatures2,
            vk::PhysicalDeviceVulkan11Features,
            vk::PhysicalDeviceVulkan13Features,
            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        > featureChain = {
            {
                .features = {
                    .samplerAnisotropy = true,
                },
            },
            {
                .shaderDrawParameters = true,
            },
            {
                .synchronization2 = true,
                .dynamicRendering = true,
            },
            {
                .extendedDynamicState = true,
            },
        };

        auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();
        std::vector<const char*> extensions(DEVICE_EXTENSIONS);
        if (std::ranges::any_of(availableExtensions, [&](const vk::ExtensionProperties& extension) {
            return strcmp(extension.extensionName, "VK_KHR_portability_subset") == 0;
        })) {
            extensions.push_back("VK_KHR_portability_subset");
        }

        float queuePriority = 0.5f;
        vk::DeviceQueueCreateInfo queueCreateInfo {
            .queueFamilyIndex = queueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &queuePriority,
        };
        device = vk::raii::Device(physicalDevice, {
            .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queueCreateInfo,
            .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
            .ppEnabledExtensionNames = extensions.data(),
        });
        queue = vk::raii::Queue(device, queueFamilyIndex, 0);
    }

    // Allocator
    {
        allocator = vma::raii::Allocator(instance.GetInstance(), device, {
            .physicalDevice = physicalDevice,
        });
    }
}

const vk::raii::PhysicalDevice& Device::GetPhysicalDevice() const {
    return physicalDevice;
}

const vk::raii::Device& Device::GetDevice() const {
    return device;
}

const vk::raii::Queue& Device::GetQueue() const {
    return queue;
}

uint32_t Device::GetQueueFamilyIndex() const {
    return queueFamilyIndex;
}

const vma::raii::Allocator& Device::GetAllocator() const {
    return allocator;
}
