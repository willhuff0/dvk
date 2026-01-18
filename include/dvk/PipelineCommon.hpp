#pragma once

#include "Shader.hpp"

#include <vulkan/vulkan_raii.hpp>

#include <memory>
#include <unordered_map>
#include <utility>

namespace dvk {
    template<class Builder>
    class StageBuilder {
    public:
        StageBuilder(Builder& builder, vk::ShaderStageFlagBits stage, const Shader& shader, const char* entry);

        StageBuilder& SetConstant(uint32_t id, const void* src, size_t size);
        template<class T>
        StageBuilder& SetConstant(uint32_t id, T value);
        StageBuilder& SetConstant(uint32_t id, bool value);

        Builder& FinishStage();

    private:
        Builder& builder;
        vk::PipelineShaderStageCreateInfo stageInfo;
        std::unordered_map<uint32_t, std::pair<std::unique_ptr<unsigned char[]>, size_t>> specialization;
        size_t totalSpecializationMapEntries = 0;
        size_t specializationDataSize = 0;
    };

    template<class Builder>
    StageBuilder<Builder>::StageBuilder(Builder& builder, vk::ShaderStageFlagBits stage, const Shader& shader, const char* entry) : builder(builder) {
        stageInfo.stage = stage;
        stageInfo.module = shader.GetShaderModule();
        stageInfo.pName = entry;
    }

    template<class Builder>
    StageBuilder<Builder>& StageBuilder<Builder>::SetConstant(uint32_t id, const void* src, size_t size) {
        auto ptr = std::make_unique<unsigned char[]>(size);
        std::memcpy(ptr.get(), src, size);
        if (specialization.contains(id)) {
            specializationDataSize -= specialization[id].second;
        } else {
            totalSpecializationMapEntries++;
        }
        specialization[id] = std::make_pair(std::move(ptr), size);
        specializationDataSize += size;
        return *this;
    }

    template<class Builder>
    template<class T>
    StageBuilder<Builder>& StageBuilder<Builder>::SetConstant(uint32_t id, T value) {
        static_assert(!std::same_as<T, bool>);
        SetConstant(id, &value, sizeof(T));
        return *this;
    }

    template<class Builder>
    StageBuilder<Builder>& StageBuilder<Builder>::SetConstant(uint32_t id, bool value) {
        vk::Bool32 vkValue = value ? vk::True : vk::False;
        SetConstant(id, &vkValue, sizeof(vk::Bool32));
        return *this;
    }

    template<class Builder>
    Builder& StageBuilder<Builder>::FinishStage() {
        builder.addStage(stageInfo, std::move(specialization), totalSpecializationMapEntries, specializationDataSize);
        return builder;
    }
}
