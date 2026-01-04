#pragma once

#include <dvk/Config.hpp>

#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc_raii.hpp>

#include <memory>
#include <string>

namespace dvk {
    class Device;

    class Buffer {
    public:
        class Builder;
        using Ref = std::shared_ptr<Buffer>;

        static Builder Create(const Device& device);
        static Ref CreateStagingBuffer(const Device& device, vk::DeviceSize size);
        explicit Buffer(vma::raii::Buffer buffer);

        [[nodiscard]] const vma::raii::Buffer& GetBuffer() const;
        [[nodiscard]] void* GetMappedData() const;

        [[nodiscard]] void* Map() const;
        void Unmap() const;

    private:
        vma::raii::Buffer buffer = nullptr;

    public:
        class Builder {
        public:
            explicit Builder(const Device& device);

            Builder& SetLabel(const std::string& label);
            Builder& SetSize(vk::DeviceSize size);
            Builder& SetUsage(vk::BufferUsageFlags usage);
            Builder& AllowSequentialWrite(bool sequentialWrite = true);
            Builder& AllowRandomAccess(bool randomAccess = true);
            Builder& EnablePersistentMapping(bool persistentMapping = true);

            [[nodiscard]] Ref Build() const;

        private:
            const Device& device;
            vk::BufferCreateInfo bufferInfo;
            vma::AllocationCreateInfo allocationInfo;
#if defined(DVK_ENABLE_DEBUG_LABELS)
            std::string label;
#endif
        };
    };
}