#include <dvk/Instance.hpp>
#include <dvk/Surface.hpp>
#include <dvk/Device.hpp>
#include <dvk/Presenter.hpp>
#include <dvk/Shader.hpp>
#include <dvk/BindSetLayout.hpp>
#include <dvk/PipelineLayout.hpp>
#include <dvk/GraphicsPipeline.hpp>
#include <dvk/CommandEncoder.hpp>

int main() {
    dvk::Instance instance({});
    dvk::Surface surface(instance, nullptr);
    dvk::Device device(instance, surface);
    dvk::Presenter presenter(device, surface, {});
    dvk::CommandEncoder commandEncoder(device);

    auto bindSetLayout = dvk::BindSetLayout::Create(device)
        .SetLabel("Test")
        .AddEntry(0)
            .SetType(vk::DescriptorType::eUniformBuffer)
            .SetStage(vk::ShaderStageFlagBits::eVertex)
            .FinishEntry()
        .Build();

    auto pipelineLayout = dvk::PipelineLayout::Create(device)
        .SetLabel("Test")
        .AddBindSetLayout(bindSetLayout)
        .Build();

    auto vertexShader = dvk::Shader(device, {}, "Test");
    auto fragmentShader = dvk::Shader(device, {}, "Test");

    auto graphicsPipeline = dvk::GraphicsPipeline::Create(device, pipelineLayout)
        .AddStage(vk::ShaderStageFlagBits::eVertex, vertexShader, "vertexEntry")
        .AddStage(vk::ShaderStageFlagBits::eFragment, fragmentShader, "fragmentEntry")
        .SetSingleAttachment(vk::Format::eR8G8B8A8Unorm)
        .SetVertexInput({}, {})
        .EnableMultisample(vk::SampleCountFlagBits::e16)
        .EnableDepth(vk::Format::eD32Sfloat)
        .Build();

    while (true) {
        auto& onSurfaceReadySemaphore = commandEncoder.WaitForNextFrame();
        auto [result, imageIndex, imageView, onRenderFinishedSemaphore] = presenter.GetNextImage(onSurfaceReadySemaphore);
        switch (result) {
            case dvk::PresenterResult::Fail_MustResize:
                //
                break;
            default:
                break;
        }
        auto [frameIndex, commandBuffer] = commandEncoder.GetNextFrame();

        // Render

        commandEncoder.SubmitFrame(frameIndex, onRenderFinishedSemaphore);
        result = presenter.Present(imageIndex);
        switch (result) {
            case dvk::PresenterResult::Fail_MustResize:
                //
                break;
            default:
                break;
        }
    }

    return 0;
}
