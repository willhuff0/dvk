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
        // Queue for async uploads. Prefers a dedicated transfer-only family (DMA engine), then a second
        // queue in the graphics family. If neither exists this aliases GetQueue(), and callers must
        // externally synchronize submissions with the render thread (see TransferQueueAliasesGraphics()).
        [[nodiscard]] const vk::raii::Queue& GetTransferQueue() const;
        [[nodiscard]] uint32_t GetTransferQueueFamilyIndex() const;
        [[nodiscard]] bool TransferQueueAliasesGraphics() const;
        [[nodiscard]] const vma::raii::Allocator& GetAllocator() const;

    private:
        vk::raii::PhysicalDevice physicalDevice = nullptr;
        vk::raii::Device device = nullptr;
        vk::raii::Queue queue = nullptr;
        uint32_t queueFamilyIndex = ~0;
        vk::raii::Queue transferQueue = nullptr;
        uint32_t transferQueueFamilyIndex = ~0;
        bool transferQueueAliasesGraphics = false;
        vma::raii::Allocator allocator = nullptr;
    };
}