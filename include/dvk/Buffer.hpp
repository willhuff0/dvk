#pragma once

#include <dvk/Config.hpp>
#include "Device.hpp"

#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc_raii.hpp>

#include <memory>
#include <string>
#include <array>

namespace dvk {
    class CommandEncoder;

    class Buffer {
    public:
        class Builder;
        using Ref = std::shared_ptr<Buffer>;
        template<size_t N>
        using Refs = std::array<Ref, N>;

        static Builder Create(const Device& device);
        static Ref CreateStagingBuffer(const Device& device, vk::DeviceSize size);
        Buffer(vma::raii::Buffer buffer, vk::DeviceSize size);

        [[nodiscard]] const vma::raii::Buffer& GetBuffer() const;
        [[nodiscard]] vk::DeviceSize GetSize() const;

        [[nodiscard]] void* GetMappedData() const;
        [[nodiscard]] void* Map() const;
        void Unmap() const;
        void Flush(vk::DeviceSize offset = 0, vk::DeviceSize size = 0) const;

        void CopyToAndFlush(const void* src, vk::DeviceSize offset = 0, vk::DeviceSize size = 0);
        void WriteWithStaging(const CommandEncoder& commandEncoder, const void* src, vk::DeviceSize offset = 0, vk::DeviceSize size = 0);

    private:
        vma::raii::Buffer buffer = nullptr;
        vk::DeviceSize size;

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
            Builder& EnableCopyToAndFlush();
            Builder& EnableWriteWithStaging();

            [[nodiscard]] Ref Build() const;
            template<size_t N>
            [[nodiscard]] Refs<N> Build() const;

        private:
            const Device& device;
            vk::BufferCreateInfo bufferInfo;
            vma::AllocationCreateInfo allocationInfo;
#if defined(DVK_ENABLE_DEBUG_LABELS)
            std::string label;
#endif
        };
    };

    template<size_t N>
    Buffer::Refs<N> Buffer::Builder::Build() const {
        std::array<Ref, N> buffers;

        for (size_t i = 0; i < N; i++) {
            auto buffer = vma::raii::Buffer(device.GetAllocator(), bufferInfo, allocationInfo);

#if defined(DVK_ENABLE_DEBUG_LABELS)
            if (!label.empty()) device.GetDevice().setDebugUtilsObjectNameEXT(*buffer, label + " (" + std::to_string(N) + ")");
#endif

            buffers[i] = std::make_shared<Buffer>(std::move(buffer), bufferInfo.size);
        }

        return buffers;
    }
}
