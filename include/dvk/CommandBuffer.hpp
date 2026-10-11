#pragma once

#include "Buffer.hpp"
#include "Texture.hpp"
#include "GraphicsPipeline.hpp"
#include "ComputePipeline.hpp"
#include "PipelineLayout.hpp"
#include "BindSet.hpp"

#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>

#include <vector>
#include <optional>
#include <functional>

namespace dvk {
    class CommandBuffer {
    public:
        class BarrierBuilder;
        class MemoryBarrierBuilder;
        class BufferBarrierBuilder;
        class ImageBarrierBuilder;
        class BufferCopyBuilder;
        class RenderingBuilder;
        class RenderingAttachmentBuilder;
        explicit CommandBuffer(vk::raii::CommandBuffer commandBuffer);

        [[nodiscard]] const vk::raii::CommandBuffer& GetCommandBuffer() const;

        CommandBuffer& InsertLabel(const std::string& label, glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        CommandBuffer& BeginRegion(const std::string& label, glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        CommandBuffer& EndRegion();

        BarrierBuilder InsertBarriers();

        CommandBuffer& FillBuffer(const Buffer::Ref& buffer, uint32_t data, vk::DeviceSize offset = 0, vk::DeviceSize length = vk::WholeSize);
        BufferCopyBuilder CopyBuffer(const Buffer::Ref& from, const Buffer::Ref& to);

        RenderingBuilder BeginRendering();
        CommandBuffer& EndRendering();

        CommandBuffer& SetViewport(glm::vec2 offset, glm::vec2 extent, float minDepth = 0.0f, float maxDepth = 1.0f);
        CommandBuffer& SetViewport(const std::vector<vk::Viewport>& viewports);

        CommandBuffer& SetScissor(vk::Offset2D offset, vk::Extent2D extent);
        CommandBuffer& SetScissor(const std::vector<vk::Rect2D>& scissors);

        CommandBuffer& SetGraphicsPipeline(const GraphicsPipeline::Ref& pipeline);
        CommandBuffer& SetComputePipeline(const ComputePipeline::Ref& pipeline);

        CommandBuffer& SetBindSet(uint32_t binding, const BindSet::Ref& bindSet);

        CommandBuffer& SetVertexBuffer(uint32_t location, const Buffer::Ref& buffer, vk::DeviceSize offset = 0);
        CommandBuffer& SetIndexBuffer(const Buffer::Ref& buffer, vk::IndexType indexType = vk::IndexType::eUint32, vk::DeviceSize offset = 0);

        template<class T>
        CommandBuffer& PushConstant(uint32_t id, vk::ShaderStageFlags stage, const T& value);

        CommandBuffer& Draw(uint32_t vertexCount, uint32_t vertexOffset = 0);
        CommandBuffer& DrawInstances(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset = 0, uint32_t instanceOffset = 0);
        CommandBuffer& DrawIndexed(uint32_t indexCount, uint32_t indexOffset = 0, int32_t vertexOffset = 0);
        CommandBuffer& DrawInstancesIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset = 0, int32_t vertexOffset = 0, uint32_t instanceOffset = 0);
        CommandBuffer& DrawIndirect(const Buffer::Ref& buffer, uint32_t drawCount, vk::DeviceSize offset = 0, uint32_t stride = sizeof(VkDrawIndirectCommand));
        CommandBuffer& DrawIndexedIndirect(const Buffer::Ref& buffer, uint32_t drawCount, vk::DeviceSize offset = 0, uint32_t stride = sizeof(VkDrawIndexedIndirectCommand));
        CommandBuffer& DrawIndirectCount(const Buffer::Ref& indirectBuffer, const Buffer::Ref& countBuffer, uint32_t maxDrawCount, vk::DeviceSize indirectBufferOffset = 0, vk::DeviceSize countBufferOffset = 0, uint32_t indirectBufferStride = sizeof(VkDrawIndirectCommand));
        CommandBuffer& DrawIndexedIndirectCount(const Buffer::Ref& indirectBuffer, const Buffer::Ref& countBuffer, uint32_t maxDrawCount, vk::DeviceSize indirectBufferOffset = 0, vk::DeviceSize countBufferOffset = 0, uint32_t indirectBufferStride = sizeof(VkDrawIndexedIndirectCommand));

        CommandBuffer& Dispatch(uint32_t xCount, uint32_t yCount = 1, uint32_t zCount = 1);
        CommandBuffer& DispatchIndirect(const Buffer::Ref& buffer, vk::DeviceSize offset = 0);

    private:
        void insertBarrier(const vk::DependencyInfo& dependencyInfo) const;
        void copyBuffer(const vk::raii::Buffer& src, const vk::raii::Buffer& dst, const std::vector<vk::BufferCopy>& regions);
        void beginRendering(const vk::RenderingInfo& renderingInfo);

        vk::raii::CommandBuffer commandBuffer;
        vk::PipelineBindPoint bindPoint;
        PipelineLayout::Ref pipelineLayout;

    public:
        class BarrierBuilder {
            friend class MemoryBarrierBuilder;
            friend class BufferBarrierBuilder;
            friend class ImageBarrierBuilder;
        public:
            explicit BarrierBuilder(CommandBuffer& commandBuffer);

            MemoryBarrierBuilder AddMemoryBarrier();
            BufferBarrierBuilder AddBufferBarrier();
            ImageBarrierBuilder AddImageBarrier();

            CommandBuffer& FinishBarriers();

        private:
            void addMemoryBarrier(vk::MemoryBarrier2 memoryBarrier);
            void addBufferBarrier(vk::BufferMemoryBarrier2 bufferBarrier);
            void addImageBarrier(vk::ImageMemoryBarrier2 imageBarrier);

            CommandBuffer& commandBuffer;
            std::vector<vk::MemoryBarrier2> memoryBarriers;
            std::vector<vk::BufferMemoryBarrier2> bufferBarriers;
            std::vector<vk::ImageMemoryBarrier2> imageBarriers;
        };

        class MemoryBarrierBuilder {
        public:
            explicit MemoryBarrierBuilder(BarrierBuilder& builder);

            MemoryBarrierBuilder& SetStages(vk::PipelineStageFlags2 src, vk::PipelineStageFlags2 dst);
            MemoryBarrierBuilder& SetAccess(vk::AccessFlags2 src, vk::AccessFlags2 dst);

            [[nodiscard]] BarrierBuilder& FinishMemoryBarrier() const;

        private:
            BarrierBuilder& builder;
            vk::MemoryBarrier2 memoryBarrier;
        };

        class BufferBarrierBuilder {
        public:
            explicit BufferBarrierBuilder(BarrierBuilder& builder);

            BufferBarrierBuilder& SetBuffer(const Buffer::Ref& buffer);
            BufferBarrierBuilder& SetBuffer(const vk::raii::Buffer& buffer);
            BufferBarrierBuilder& SetRange(vk::DeviceSize offset, vk::DeviceSize size);
            BufferBarrierBuilder& SetQueues(uint32_t srcQueueFamily, uint32_t dstQueueFamily);
            BufferBarrierBuilder& SetStages(vk::PipelineStageFlags2 src, vk::PipelineStageFlags2 dst);
            BufferBarrierBuilder& SetAccess(vk::AccessFlags2 src, vk::AccessFlags2 dst);

            [[nodiscard]] BarrierBuilder& FinishBufferBarrier() const;

        private:
            BarrierBuilder& builder;
            vk::BufferMemoryBarrier2 bufferBarrier;
        };

        class ImageBarrierBuilder {
        public:
            explicit ImageBarrierBuilder(BarrierBuilder& builder);

            ImageBarrierBuilder& SetImage(const Texture::Ref& texture);
            ImageBarrierBuilder& SetImage(const vk::Image& image);
            ImageBarrierBuilder& SetAspect(vk::ImageAspectFlags aspect);
            ImageBarrierBuilder& SetMipLevels(uint32_t base, uint32_t count = 1);
            ImageBarrierBuilder& SetArrayLayers(uint32_t base, uint32_t count = 1);
            ImageBarrierBuilder& SetLayout(vk::ImageLayout from, vk::ImageLayout to);
            ImageBarrierBuilder& SetQueues(uint32_t srcQueueFamily, uint32_t dstQueueFamily);
            ImageBarrierBuilder& SetStages(vk::PipelineStageFlags2 src, vk::PipelineStageFlags2 dst);
            ImageBarrierBuilder& SetAccess(vk::AccessFlags2 src, vk::AccessFlags2 dst);
            ImageBarrierBuilder& InferStages();
            ImageBarrierBuilder& InferAccess();
            ImageBarrierBuilder& InferStagesAndAccess();

            [[nodiscard]] BarrierBuilder& FinishImageBarrier() const;

        private:
            BarrierBuilder& builder;
            vk::ImageMemoryBarrier2 imageBarrier;
        };

        class BufferCopyBuilder {
        public:
            explicit BufferCopyBuilder(CommandBuffer& commandBuffer, Buffer::Ref from, Buffer::Ref to);

            BufferCopyBuilder& SetBuffers(Buffer::Ref src, Buffer::Ref to);
            BufferCopyBuilder& AddRegion(vk::DeviceSize fromOffset, vk::DeviceSize toOffset, vk::DeviceSize size);

            CommandBuffer& FinishBufferCopy();

        private:
            CommandBuffer& commandBuffer;
            Buffer::Ref from;
            Buffer::Ref to;
            std::vector<vk::BufferCopy> regions;
        };

        class RenderingBuilder {
            friend class RenderingAttachmentBuilder;
        public:
            explicit RenderingBuilder(CommandBuffer& commandBuffer);

            RenderingBuilder& SetRenderArea(vk::Offset2D offset, vk::Extent2D extent);
            RenderingAttachmentBuilder AddColorAttachment();
            RenderingAttachmentBuilder SetDepthAttachment();
            RenderingAttachmentBuilder SetStencilAttachment();
            RenderingBuilder& SetColorAttachmentSimple(const TextureView::Ref& view);
            RenderingBuilder& SetColorAttachmentSimple(const vk::raii::ImageView& view);
            RenderingBuilder& SetDepthAttachmentSimple(const TextureView::Ref& view, bool clear = true);
            RenderingBuilder& SetDepthAttachmentSimple(const vk::raii::ImageView& view, bool clear = true);

            CommandBuffer& FinishBeginRendering();

        private:
            void setRenderExtentIfEmpty(vk::Extent2D extent);

            CommandBuffer& commandBuffer;
            std::optional<vk::Rect2D> areaInfo;
            std::vector<vk::RenderingAttachmentInfo> colorInfos;
            std::optional<vk::RenderingAttachmentInfo> depthInfo;
            std::optional<vk::RenderingAttachmentInfo> stencilInfo;
        };

        class RenderingAttachmentBuilder {
        public:
            RenderingAttachmentBuilder(RenderingBuilder& builder, std::function<void(RenderingBuilder& builder, vk::RenderingAttachmentInfo info)> callback);

            RenderingAttachmentBuilder& SetView(const TextureView::Ref& view);
            RenderingAttachmentBuilder& SetView(const vk::raii::ImageView& view);
            RenderingAttachmentBuilder& SetImageLayout(vk::ImageLayout imageLayout);
            RenderingAttachmentBuilder& SetResolveMode(vk::ResolveModeFlagBits resolveMode);
            RenderingAttachmentBuilder& SetResolveView(const TextureView::Ref& resolveView);
            RenderingAttachmentBuilder& SetResolveView(const vk::raii::ImageView& resolveView);
            RenderingAttachmentBuilder& SetResolveImageLayout(vk::ImageLayout resolveImageLayout);
            RenderingAttachmentBuilder& EnableMultisample(const TextureView::Ref& resolveView);
            RenderingAttachmentBuilder& EnableMultisample(const vk::raii::ImageView& resolveView);
            RenderingAttachmentBuilder& SetOps(vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp);
            RenderingAttachmentBuilder& SetClearValue(vk::ClearValue clearValue);

            RenderingBuilder& FinishAttachment();

        private:
            RenderingBuilder& builder;
            std::function<void(RenderingBuilder& builder, vk::RenderingAttachmentInfo info)> callback;
            vk::RenderingAttachmentInfo attachmentInfo;
        };
    };

    template<class T>
    CommandBuffer& CommandBuffer::PushConstant(uint32_t id, vk::ShaderStageFlags stage, const T& value) {
        commandBuffer.pushConstants<T>(pipelineLayout->GetPipelineLayout(), stage, id, { value });
        return *this;
    }
}
