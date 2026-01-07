#include <dvk/Buffer.hpp>
#include "dvk/CommandEncoder.hpp"

#include <utility>

using namespace dvk;

// Buffer
Buffer::Builder Buffer::Create(const Device& device) {
    return Builder(device);
}

Buffer::Ref Buffer::CreateStagingBuffer(const Device& device, vk::DeviceSize size) {
    return std::make_shared<Buffer>(vma::raii::Buffer(device.GetAllocator(),
        vk::BufferCreateInfo {
            .size = size,
            .usage = vk::BufferUsageFlagBits::eTransferSrc,
            .sharingMode = vk::SharingMode::eExclusive,
        },
        vma::AllocationCreateInfo {
            .flags = vma::AllocationCreateFlagBits::eHostAccessSequentialWrite | vma::AllocationCreateFlagBits::eMapped,
            .usage = vma::MemoryUsage::eAuto,
        }
    ), size);
}

Buffer::Buffer(vma::raii::Buffer buffer, vk::DeviceSize size) : buffer(std::move(buffer)), size(size) { }

const vma::raii::Buffer& Buffer::GetBuffer() const {
    return buffer;
}

vk::DeviceSize Buffer::GetSize() const {
    return size;
}

void* Buffer::GetMappedData() const {
    return buffer.getAllocation().getInfo().pMappedData;
}

void* Buffer::Map() const {
    return buffer.getAllocation().map();
}

void Buffer::Unmap() const {
    buffer.getAllocation().unmap();
}

void Buffer::Flush(vk::DeviceSize offset , vk::DeviceSize size_) const {
    if (size_ == 0) size_ = size;

    buffer.getAllocation().flush(offset, size_);
}

void Buffer::CopyToAndFlush(const void* src, vk::DeviceSize offset, vk::DeviceSize size_) {
    if (size_ == 0) size_ = size;

    std::memcpy(static_cast<char*>(GetMappedData()) + offset, src, size_);
    Flush(offset, size_);
}

void Buffer::WriteWithStaging(const CommandEncoder& commandEncoder, const void* src, vk::DeviceSize offset, vk::DeviceSize size_) {
    if (size_ == 0) size_ = size;

    auto stagingBuffer = Buffer::CreateStagingBuffer(commandEncoder.GetDevice(), size_);
    std::memcpy(stagingBuffer->GetMappedData(), src, size_);

    commandEncoder.ExecuteOneTime([&](const auto& commandBuffer) {
        commandBuffer.copyBuffer(stagingBuffer->GetBuffer(), buffer, vk::BufferCopy(0, offset, size_));
    });
}

// Buffer Builder
Buffer::Builder::Builder(const Device& device) : device(device) {
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    allocationInfo.usage = vma::MemoryUsage::eAuto;
}

Buffer::Builder& Buffer::Builder::SetLabel(const std::string& label_) {
#if defined(DVK_ENABLE_DEBUG_LABELS)
    label = label_;
#endif
    return *this;
}

Buffer::Builder& Buffer::Builder::SetSize(vk::DeviceSize size) {
    bufferInfo.size = size;
    return *this;
}

Buffer::Builder& Buffer::Builder::SetUsage(vk::BufferUsageFlags usage) {
    bufferInfo.usage = usage;
    return *this;
}

Buffer::Builder& Buffer::Builder::AllowSequentialWrite(bool sequentialWrite) {
    if (sequentialWrite) {
        allocationInfo.flags |= vma::AllocationCreateFlagBits::eHostAccessSequentialWrite;
    } else {
        allocationInfo.flags &= ~vma::AllocationCreateFlagBits::eHostAccessSequentialWrite;
    }
    return *this;
}

Buffer::Builder& Buffer::Builder::AllowRandomAccess(bool randomAccess) {
    if (randomAccess) {
        allocationInfo.flags |= vma::AllocationCreateFlagBits::eHostAccessRandom;
    } else {
        allocationInfo.flags &= ~vma::AllocationCreateFlagBits::eHostAccessRandom;
    }
    return *this;
}

Buffer::Builder& Buffer::Builder::EnablePersistentMapping(bool persistentMapping) {
    if (persistentMapping) {
        allocationInfo.flags |= vma::AllocationCreateFlagBits::eMapped;
    } else {
        allocationInfo.flags &= ~vma::AllocationCreateFlagBits::eMapped;
    }
    return *this;
}

Buffer::Builder& Buffer::Builder::EnableCopyToAndFlush() {
    allocationInfo.flags |= vma::AllocationCreateFlagBits::eMapped | vma::AllocationCreateFlagBits::eHostAccessRandom;
    return *this;
}

Buffer::Builder& Buffer::Builder::EnableWriteWithStaging() {
    bufferInfo.usage |= vk::BufferUsageFlagBits::eTransferDst;
    return *this;
}

Buffer::Ref Buffer::Builder::Build() const {
    auto buffer = vma::raii::Buffer(device.GetAllocator(), bufferInfo, allocationInfo);

#if defined(DVK_ENABLE_DEBUG_LABELS)
    if (!label.empty()) device.GetDevice().setDebugUtilsObjectNameEXT(*buffer, label);
#endif

    return std::make_shared<Buffer>(std::move(buffer), bufferInfo.size);
}
