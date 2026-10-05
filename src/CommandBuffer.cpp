#include <dvk/CommandBuffer.hpp>

#include <utility>
#include <stdexcept>

using namespace dvk;

// CommandBuffer
CommandBuffer::CommandBuffer(vk::raii::CommandBuffer commandBuffer)
    : commandBuffer(std::move(commandBuffer)) {
    bindPoint = vk::PipelineBindPoint::eGraphics;
}

const vk::raii::CommandBuffer& CommandBuffer::GetCommandBuffer() const {
    return commandBuffer;
}

CommandBuffer::BarrierBuilder CommandBuffer::InsertBarriers() {
    return BarrierBuilder(*this);
}

CommandBuffer& CommandBuffer::FillBuffer(const Buffer::Ref& buffer, uint32_t data, vk::DeviceSize offset, vk::DeviceSize length) {
    commandBuffer.fillBuffer(buffer->GetBuffer(), offset, length, data);
    return *this;
}

CommandBuffer::RenderingBuilder CommandBuffer::BeginRendering() {
    return RenderingBuilder(*this);
}

CommandBuffer& CommandBuffer::EndRendering() {
    commandBuffer.endRendering();
    return *this;
}

CommandBuffer& CommandBuffer::SetViewport(glm::vec2 offset, glm::vec2 extent, float minDepth, float maxDepth) {
    commandBuffer.setViewport(0, { vk::Viewport {
        .x = offset.x,
        .y = offset.y,
        .width = extent.x,
        .height = extent.y,
        .minDepth = minDepth,
        .maxDepth = maxDepth,
    }});
    return *this;
}

CommandBuffer& CommandBuffer::SetViewport(const std::vector<vk::Viewport>& viewports) {
    commandBuffer.setViewport(0, viewports);
    return *this;
}

CommandBuffer& CommandBuffer::SetScissor(vk::Offset2D offset, vk::Extent2D extent) {
    commandBuffer.setScissor(0, { vk::Rect2D {
        .offset = offset,
        .extent = extent,
    }});
    return *this;
}

CommandBuffer& CommandBuffer::SetScissor(const std::vector<vk::Rect2D>& scissors) {
    commandBuffer.setScissor(0, scissors);
    return *this;
}

CommandBuffer& CommandBuffer::SetGraphicsPipeline(const dvk::GraphicsPipeline::Ref& pipeline) {
    bindPoint = vk::PipelineBindPoint::eGraphics;
    pipelineLayout = pipeline->GetLayout();
    commandBuffer.bindPipeline(bindPoint, pipeline->GetPipeline());
    return *this;
}

CommandBuffer& CommandBuffer::SetComputePipeline(const dvk::ComputePipeline::Ref& pipeline) {
    bindPoint = vk::PipelineBindPoint::eCompute;
    pipelineLayout = pipeline->GetLayout();
    commandBuffer.bindPipeline(bindPoint, pipeline->GetPipeline());
    return *this;
}

CommandBuffer& CommandBuffer::SetBindSet(uint32_t binding, const dvk::BindSet::Ref& bindSet) {
    commandBuffer.bindDescriptorSets(bindPoint, pipelineLayout->GetPipelineLayout(), binding, *bindSet->GetDescriptorSet(), nullptr);
    return *this;
}

CommandBuffer& CommandBuffer::SetVertexBuffer(uint32_t location, const Buffer::Ref& buffer, vk::DeviceSize offset) {
    commandBuffer.bindVertexBuffers(0, *buffer->GetBuffer(), { offset });
    return *this;
}

CommandBuffer& CommandBuffer::SetIndexBuffer(const Buffer::Ref& buffer, vk::IndexType indexType, vk::DeviceSize offset) {
    commandBuffer.bindIndexBuffer(buffer->GetBuffer(), offset, indexType);
    return *this;
}

CommandBuffer& CommandBuffer::Draw(uint32_t vertexCount, uint32_t vertexOffset) {
    commandBuffer.draw(vertexCount, 1, vertexOffset, 0);
    return *this;
}

CommandBuffer& CommandBuffer::DrawInstances(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset, uint32_t instanceOffset) {
    commandBuffer.draw(vertexCount, instanceCount, vertexOffset, instanceOffset);
    return *this;
}

CommandBuffer& CommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t indexOffset, int32_t vertexOffset) {
    commandBuffer.drawIndexed(indexCount, 1, indexOffset, vertexOffset, 0);
    return *this;
}

CommandBuffer& CommandBuffer::DrawInstancesIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, int32_t vertexOffset, uint32_t instanceOffset) {
    commandBuffer.drawIndexed(indexCount, instanceCount, indexOffset, vertexOffset, instanceOffset);
    return *this;
}

CommandBuffer& CommandBuffer::DrawIndirect(const Buffer::Ref& buffer, uint32_t drawCount, vk::DeviceSize offset, uint32_t stride) {
    commandBuffer.drawIndirect(buffer->GetBuffer(), offset, drawCount, stride);
    return *this;
}

CommandBuffer& CommandBuffer::DrawIndexedIndirect(const Buffer::Ref& buffer, uint32_t drawCount, vk::DeviceSize offset, uint32_t stride) {
    commandBuffer.drawIndexedIndirect(buffer->GetBuffer(), offset, drawCount, stride);
    return *this;
}

CommandBuffer& CommandBuffer::DrawIndirectCount(const Buffer::Ref& indirectBuffer, const Buffer::Ref& countBuffer, uint32_t maxDrawCount, vk::DeviceSize indirectBufferOffset, vk::DeviceSize countBufferOffset, uint32_t indirectBufferStride) {
    commandBuffer.drawIndirectCount(indirectBuffer->GetBuffer(), indirectBufferOffset, countBuffer->GetBuffer(), countBufferOffset, maxDrawCount, indirectBufferStride);
    return *this;
}

CommandBuffer& CommandBuffer::DrawIndexedIndirectCount(const Buffer::Ref& indirectBuffer, const Buffer::Ref& countBuffer, uint32_t maxDrawCount, vk::DeviceSize indirectBufferOffset, vk::DeviceSize countBufferOffset, uint32_t indirectBufferStride) {
    commandBuffer.drawIndexedIndirectCount(indirectBuffer->GetBuffer(), indirectBufferOffset, countBuffer->GetBuffer(), countBufferOffset, maxDrawCount, indirectBufferStride);
    return *this;
}

CommandBuffer& CommandBuffer::Dispatch(uint32_t xCount, uint32_t yCount, uint32_t zCount) {
    commandBuffer.dispatch(xCount, yCount, zCount);
    return *this;
}

CommandBuffer& CommandBuffer::DispatchIndirect(const Buffer::Ref& buffer, vk::DeviceSize offset) {
    commandBuffer.dispatchIndirect(buffer->GetBuffer(), offset);
    return *this;
}

void CommandBuffer::insertBarrier(const vk::DependencyInfo& dependencyInfo) const {
    commandBuffer.pipelineBarrier2(dependencyInfo);
}

void CommandBuffer::beginRendering(const vk::RenderingInfo& renderingInfo) {
    commandBuffer.beginRendering(renderingInfo);
}

// Barrier Builder
CommandBuffer::BarrierBuilder::BarrierBuilder(CommandBuffer& commandBuffer) : commandBuffer(commandBuffer) { }

CommandBuffer::MemoryBarrierBuilder CommandBuffer::BarrierBuilder::AddMemoryBarrier() {
    return MemoryBarrierBuilder(*this);
}

CommandBuffer::BufferBarrierBuilder CommandBuffer::BarrierBuilder::AddBufferBarrier() {
    return BufferBarrierBuilder(*this);
}

CommandBuffer::ImageBarrierBuilder CommandBuffer::BarrierBuilder::AddImageBarrier() {
    return ImageBarrierBuilder(*this);
}

void CommandBuffer::BarrierBuilder::addMemoryBarrier(vk::MemoryBarrier2 memoryBarrier) {
    memoryBarriers.emplace_back(std::move(memoryBarrier));
}

void CommandBuffer::BarrierBuilder::addBufferBarrier(vk::BufferMemoryBarrier2 bufferBarrier) {
    bufferBarriers.emplace_back(std::move(bufferBarrier));
}

void CommandBuffer::BarrierBuilder::addImageBarrier(vk::ImageMemoryBarrier2 imageBarrier) {
    imageBarriers.emplace_back(std::move(imageBarrier));
}

void CommandBuffer::BarrierBuilder::FinishBarriers() const {
    commandBuffer.insertBarrier({
        .memoryBarrierCount = static_cast<uint32_t>(memoryBarriers.size()),
        .pMemoryBarriers = memoryBarriers.data(),
        .bufferMemoryBarrierCount = static_cast<uint32_t>(bufferBarriers.size()),
        .pBufferMemoryBarriers = bufferBarriers.data(),
        .imageMemoryBarrierCount = static_cast<uint32_t>(imageBarriers.size()),
        .pImageMemoryBarriers = imageBarriers.data(),
    });
}

// PipelineBarrier Builder
CommandBuffer::MemoryBarrierBuilder::MemoryBarrierBuilder(BarrierBuilder& builder) : builder(builder) { }

CommandBuffer::MemoryBarrierBuilder& CommandBuffer::MemoryBarrierBuilder::SetStages(vk::PipelineStageFlags2 src, vk::PipelineStageFlags2 dst) {
    memoryBarrier.srcStageMask = src;
    memoryBarrier.dstStageMask = dst;
    return *this;
}

CommandBuffer::MemoryBarrierBuilder& CommandBuffer::MemoryBarrierBuilder::SetAccess(vk::AccessFlags2 src, vk::AccessFlags2 dst) {
    memoryBarrier.srcAccessMask = src;
    memoryBarrier.dstAccessMask = dst;
    return *this;
}

CommandBuffer::BarrierBuilder& CommandBuffer::MemoryBarrierBuilder::FinishMemoryBarrier() const {
    builder.addMemoryBarrier(std::move(memoryBarrier));
    return builder;
}

// BufferBarrier Builder
CommandBuffer::BufferBarrierBuilder::BufferBarrierBuilder(BarrierBuilder& builder) : builder(builder) {
    bufferBarrier.offset = 0;
    bufferBarrier.size = vk::WholeSize;
}

CommandBuffer::BufferBarrierBuilder& CommandBuffer::BufferBarrierBuilder::SetBuffer(const dvk::Buffer::Ref& buffer) {
    bufferBarrier.buffer = buffer->GetBuffer();
    return *this;
}
CommandBuffer::BufferBarrierBuilder& CommandBuffer::BufferBarrierBuilder::SetBuffer(const vk::raii::Buffer& buffer) {
    bufferBarrier.buffer = buffer;
    return *this;
}

CommandBuffer::BufferBarrierBuilder& CommandBuffer::BufferBarrierBuilder::SetRange(vk::DeviceSize offset, vk::DeviceSize size) {
    bufferBarrier.offset = offset;
    bufferBarrier.size = size;
    return *this;
}

CommandBuffer::BufferBarrierBuilder& CommandBuffer::BufferBarrierBuilder::SetQueues(uint32_t srcQueueFamily, uint32_t dstQueueFamily) {
    bufferBarrier.srcQueueFamilyIndex = srcQueueFamily;
    bufferBarrier.dstQueueFamilyIndex = dstQueueFamily;
    return *this;
}

CommandBuffer::BufferBarrierBuilder& CommandBuffer::BufferBarrierBuilder::SetStages(vk::PipelineStageFlags2 src, vk::PipelineStageFlags2 dst) {
    bufferBarrier.srcStageMask = src;
    bufferBarrier.dstStageMask = dst;
    return *this;
}

CommandBuffer::BufferBarrierBuilder& CommandBuffer::BufferBarrierBuilder::SetAccess(vk::AccessFlags2 src, vk::AccessFlags2 dst) {
    bufferBarrier.srcAccessMask = src;
    bufferBarrier.dstAccessMask = dst;
    return *this;
}

CommandBuffer::BarrierBuilder& CommandBuffer::BufferBarrierBuilder::FinishBufferBarrier() const {
    builder.addBufferBarrier(std::move(bufferBarrier));
    return builder;
}

// ImageBarrier Builder
static bool shouldInferSrcStageFromDstLayout(vk::ImageLayout oldLayout, vk::ImageLayout newLayout) {
    return oldLayout == vk::ImageLayout::eUndefined
        && (newLayout == vk::ImageLayout::eColorAttachmentOptimal
         || newLayout == vk::ImageLayout::eDepthAttachmentOptimal
         || newLayout == vk::ImageLayout::eStencilAttachmentOptimal
        );
}
static vk::PipelineStageFlags2 inferStageFromLayout(vk::ImageLayout layout) {
    switch (layout) {
        case vk::ImageLayout::eUndefined: return vk::PipelineStageFlagBits2::eTopOfPipe;
        case vk::ImageLayout::eTransferDstOptimal: return vk::PipelineStageFlagBits2::eTransfer;
        case vk::ImageLayout::eColorAttachmentOptimal: return vk::PipelineStageFlagBits2::eColorAttachmentOutput;
        case vk::ImageLayout::eDepthAttachmentOptimal: return vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
        case vk::ImageLayout::eStencilAttachmentOptimal: return vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests;
        case vk::ImageLayout::eShaderReadOnlyOptimal: return vk::PipelineStageFlagBits2::ePreRasterizationShaders | vk::PipelineStageFlagBits2::eComputeShader | vk::PipelineStageFlagBits2::eFragmentShader;
        case vk::ImageLayout::eGeneral: return vk::PipelineStageFlagBits2::eComputeShader;
        default:
            throw std::invalid_argument("Cannot infer stage from layout!");
    }
}
static vk::AccessFlags2 inferAccessFromLayout(vk::ImageLayout layout) {
    switch (layout) {
        case vk::ImageLayout::eUndefined: return {};
        case vk::ImageLayout::eTransferDstOptimal: return vk::AccessFlagBits2::eTransferWrite;
        case vk::ImageLayout::eColorAttachmentOptimal: return vk::AccessFlagBits2::eColorAttachmentWrite;
        case vk::ImageLayout::eDepthAttachmentOptimal: return vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
        case vk::ImageLayout::eStencilAttachmentOptimal: return vk::AccessFlagBits2::eDepthStencilAttachmentWrite;
        case vk::ImageLayout::eShaderReadOnlyOptimal: return vk::AccessFlagBits2::eShaderRead;
        case vk::ImageLayout::eGeneral: return vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite;
        default:
            throw std::invalid_argument("Cannot infer access from layout!");
    }
}

CommandBuffer::ImageBarrierBuilder::ImageBarrierBuilder(BarrierBuilder& builder) : builder(builder) {
    imageBarrier.subresourceRange = vk::ImageSubresourceRange {
        .aspectMask = vk::ImageAspectFlagBits::eColor,
        .baseMipLevel = 0,
        .levelCount = vk::RemainingMipLevels,
        .baseArrayLayer = 0,
        .layerCount = vk::RemainingArrayLayers,
    };
}

CommandBuffer::ImageBarrierBuilder& CommandBuffer::ImageBarrierBuilder::SetImage(const Texture::Ref& texture) {
    imageBarrier.image = texture->GetImage();
    return *this;
}
CommandBuffer::ImageBarrierBuilder& CommandBuffer::ImageBarrierBuilder::SetImage(const vk::Image& image) {
    imageBarrier.image = image;
    return *this;
}

CommandBuffer::ImageBarrierBuilder& CommandBuffer::ImageBarrierBuilder::SetAspect(vk::ImageAspectFlags aspect) {
    imageBarrier.subresourceRange.aspectMask = aspect;
    return *this;
}

CommandBuffer::ImageBarrierBuilder& CommandBuffer::ImageBarrierBuilder::SetMipLevels(uint32_t base, uint32_t count) {
    imageBarrier.subresourceRange.baseMipLevel = base;
    imageBarrier.subresourceRange.levelCount = count;
    return *this;
}

CommandBuffer::ImageBarrierBuilder& CommandBuffer::ImageBarrierBuilder::SetArrayLayers(uint32_t base, uint32_t count) {
    imageBarrier.subresourceRange.baseArrayLayer = base;
    imageBarrier.subresourceRange.layerCount = count;
    return *this;
}

CommandBuffer::ImageBarrierBuilder& CommandBuffer::ImageBarrierBuilder::SetLayout(vk::ImageLayout from, vk::ImageLayout to) {
    imageBarrier.oldLayout = from;
    imageBarrier.newLayout = to;
    return *this;
}

CommandBuffer::ImageBarrierBuilder& CommandBuffer::ImageBarrierBuilder::SetQueues(uint32_t srcQueueFamily, uint32_t dstQueueFamily) {
    imageBarrier.srcQueueFamilyIndex = srcQueueFamily;
    imageBarrier.dstQueueFamilyIndex = dstQueueFamily;
    return *this;
}

CommandBuffer::ImageBarrierBuilder& CommandBuffer::ImageBarrierBuilder::SetStages(vk::PipelineStageFlags2 src, vk::PipelineStageFlags2 dst) {
    imageBarrier.srcStageMask = src;
    imageBarrier.dstStageMask = dst;
    return *this;
}

CommandBuffer::ImageBarrierBuilder& CommandBuffer::ImageBarrierBuilder::SetAccess(vk::AccessFlags2 src, vk::AccessFlags2 dst) {
    imageBarrier.srcAccessMask = src;
    imageBarrier.dstAccessMask = dst;
    return *this;
}

CommandBuffer::ImageBarrierBuilder& CommandBuffer::ImageBarrierBuilder::InferStages() {
    bool srcAsDst = shouldInferSrcStageFromDstLayout(imageBarrier.oldLayout, imageBarrier.newLayout);
    imageBarrier.srcStageMask = inferStageFromLayout(srcAsDst ? imageBarrier.newLayout : imageBarrier.oldLayout);
    imageBarrier.dstStageMask = inferStageFromLayout(imageBarrier.newLayout);
    return *this;
}

CommandBuffer::ImageBarrierBuilder& CommandBuffer::ImageBarrierBuilder::InferAccess() {
    bool srcAsDst = shouldInferSrcStageFromDstLayout(imageBarrier.oldLayout, imageBarrier.newLayout);
    imageBarrier.srcAccessMask = inferAccessFromLayout(srcAsDst ? imageBarrier.newLayout : imageBarrier.oldLayout);
    imageBarrier.dstAccessMask = inferAccessFromLayout(imageBarrier.newLayout);
    return *this;
}

CommandBuffer::ImageBarrierBuilder& CommandBuffer::ImageBarrierBuilder::InferStagesAndAccess() {
    InferStages();
    InferAccess();
    return *this;
}

CommandBuffer::BarrierBuilder& CommandBuffer::ImageBarrierBuilder::FinishImageBarrier() const {
    builder.addImageBarrier(std::move(imageBarrier));
    return builder;
}

// Rendering Builder

CommandBuffer::RenderingBuilder::RenderingBuilder(CommandBuffer& commandBuffer) : commandBuffer(commandBuffer) { }

CommandBuffer::RenderingBuilder& CommandBuffer::RenderingBuilder::SetRenderArea(vk::Offset2D offset, vk::Extent2D extent) {
    areaInfo = vk::Rect2D {
        .offset = offset,
        .extent = extent,
    };
    return *this;
}

CommandBuffer::RenderingAttachmentBuilder CommandBuffer::RenderingBuilder::AddColorAttachment() {
    return RenderingAttachmentBuilder(*this, [](RenderingBuilder& this_, vk::RenderingAttachmentInfo attachmentInfo) {
        this_.colorInfos.emplace_back(std::move(attachmentInfo));
    });
}

CommandBuffer::RenderingAttachmentBuilder CommandBuffer::RenderingBuilder::SetDepthAttachment() {
    return RenderingAttachmentBuilder(*this, [](RenderingBuilder& this_, vk::RenderingAttachmentInfo attachmentInfo) {
        this_.depthInfo = std::move(attachmentInfo);
    });
}

CommandBuffer::RenderingAttachmentBuilder CommandBuffer::RenderingBuilder::SetStencilAttachment() {
    return RenderingAttachmentBuilder(*this, [](RenderingBuilder& this_, vk::RenderingAttachmentInfo attachmentInfo) {
        this_.stencilInfo = std::move(attachmentInfo);
    });
}

CommandBuffer::RenderingBuilder& CommandBuffer::RenderingBuilder::SetColorAttachmentSimple(const TextureView::Ref& view) {
    return SetColorAttachmentSimple(view->GetImageView());
}

CommandBuffer::RenderingBuilder& CommandBuffer::RenderingBuilder::SetColorAttachmentSimple(const vk::raii::ImageView& view) {
    colorInfos.emplace_back(vk::RenderingAttachmentInfo {
        .imageView = view,
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f),
    });
    return *this;
}

CommandBuffer::RenderingBuilder& CommandBuffer::RenderingBuilder::SetDepthAttachmentSimple(const TextureView::Ref& view, bool clear) {
    return SetDepthAttachmentSimple(view->GetImageView(), clear);
}

CommandBuffer::RenderingBuilder& CommandBuffer::RenderingBuilder::SetDepthAttachmentSimple(const vk::raii::ImageView& view, bool clear) {
    depthInfo = vk::RenderingAttachmentInfo {
        .imageView = view,
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = clear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue = vk::ClearDepthStencilValue(1.0f, 0),
    };
    return *this;
}

void CommandBuffer::RenderingBuilder::setRenderExtentIfEmpty(vk::Extent2D extent) {
    if (areaInfo.has_value()) return;
    areaInfo = vk::Rect2D {
        .offset = vk::Offset2D(0, 0),
        .extent = extent,
    };
}

CommandBuffer& CommandBuffer::RenderingBuilder::FinishBeginRendering() {
    commandBuffer.beginRendering(vk::RenderingInfo {
        .renderArea = areaInfo.value(),
        .layerCount = 1,
        .viewMask = 0,
        .colorAttachmentCount = static_cast<uint32_t>(colorInfos.size()),
        .pColorAttachments = colorInfos.data(),
        .pDepthAttachment = depthInfo.has_value() ? &depthInfo.value() : nullptr,
        .pStencilAttachment = stencilInfo.has_value() ? &stencilInfo.value() : nullptr,
    });
    return commandBuffer;
}

// Rendering Attachment Builder
CommandBuffer::RenderingAttachmentBuilder::RenderingAttachmentBuilder(RenderingBuilder& builder, std::function<void(RenderingBuilder& builder, vk::RenderingAttachmentInfo info)> callback)
    : builder(builder), callback(std::move(callback)) {
    attachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    attachmentInfo.resolveImageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    attachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
    attachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
    attachmentInfo.clearValue = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
}

CommandBuffer::RenderingAttachmentBuilder& CommandBuffer::RenderingAttachmentBuilder::SetView(const TextureView::Ref& view) {
    attachmentInfo.imageView = view->GetImageView();
    builder.setRenderExtentIfEmpty(vk::Extent2D(view->GetTexture()->GetExtent().width, view->GetTexture()->GetExtent().height));
    return *this;
}

CommandBuffer::RenderingAttachmentBuilder& CommandBuffer::RenderingAttachmentBuilder::SetView(const vk::raii::ImageView& view) {
    attachmentInfo.imageView = view;
    return *this;
}

CommandBuffer::RenderingAttachmentBuilder& CommandBuffer::RenderingAttachmentBuilder::SetImageLayout(vk::ImageLayout imageLayout) {
    attachmentInfo.imageLayout = imageLayout;
    return *this;
}

CommandBuffer::RenderingAttachmentBuilder& CommandBuffer::RenderingAttachmentBuilder::SetResolveMode(vk::ResolveModeFlagBits resolveMode) {
    attachmentInfo.resolveMode = resolveMode;
    return *this;
}

CommandBuffer::RenderingAttachmentBuilder& CommandBuffer::RenderingAttachmentBuilder::SetResolveView(const TextureView::Ref& resolveView) {
    attachmentInfo.resolveImageView = resolveView->GetImageView();
    return *this;
}

CommandBuffer::RenderingAttachmentBuilder& CommandBuffer::RenderingAttachmentBuilder::SetResolveView(const vk::raii::ImageView& resolveView) {
    attachmentInfo.resolveImageView = resolveView;
    return *this;
}

CommandBuffer::RenderingAttachmentBuilder& CommandBuffer::RenderingAttachmentBuilder::SetResolveImageLayout(vk::ImageLayout resolveImageLayout) {
    attachmentInfo.resolveImageLayout = resolveImageLayout;
    return *this;
}

CommandBuffer::RenderingAttachmentBuilder& CommandBuffer::RenderingAttachmentBuilder::EnableMultisample(const TextureView::Ref& resolveView) {
    attachmentInfo.resolveMode = vk::ResolveModeFlagBits::eAverage;
    attachmentInfo.resolveImageView = resolveView->GetImageView();
    return *this;
}

CommandBuffer::RenderingAttachmentBuilder& CommandBuffer::RenderingAttachmentBuilder::EnableMultisample(const vk::raii::ImageView& resolveView) {
    attachmentInfo.resolveMode = vk::ResolveModeFlagBits::eAverage;
    attachmentInfo.resolveImageView = resolveView;
    return *this;
}

CommandBuffer::RenderingAttachmentBuilder& CommandBuffer::RenderingAttachmentBuilder::SetOps(vk::AttachmentLoadOp loadOp, vk::AttachmentStoreOp storeOp) {
    attachmentInfo.loadOp = loadOp;
    attachmentInfo.storeOp = storeOp;
    return *this;
}

CommandBuffer::RenderingAttachmentBuilder& CommandBuffer::RenderingAttachmentBuilder::SetClearValue(vk::ClearValue clearValue) {
    attachmentInfo.clearValue = clearValue;
    return *this;
}

CommandBuffer::RenderingBuilder& CommandBuffer::RenderingAttachmentBuilder::FinishAttachment() {
    callback(builder, std::move(attachmentInfo));
    return builder;
}
