cmake_minimum_required(VERSION 4.1)
include(FetchContent)
project(dvk)

set(CMAKE_CXX_STANDARD 20)

add_library(dvk)

target_sources(dvk
    PRIVATE
        test.cpp
        src/Instance.cpp
        src/Surface.cpp
        src/Device.cpp
        src/Presenter.cpp
        src/Shader.cpp
        src/BindSetLayout.cpp
        src/PipelineLayout.cpp
        src/GraphicsPipeline.cpp
        src/CommandEncoder.cpp
        src/Buffer.cpp
        src/Texture.cpp
        src/TextureView.cpp
        src/Sampler.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/dvk/Config.hpp
            include/dvk/Instance.hpp
            include/dvk/Surface.hpp
            include/dvk/Device.hpp
            include/dvk/Presenter.hpp
            include/dvk/Shader.hpp
            include/dvk/BindSetLayout.hpp
            include/dvk/PipelineLayout.hpp
            include/dvk/GraphicsPipeline.hpp
            include/dvk/CommandEncoder.hpp
            include/dvk/Buffer.hpp
            include/dvk/TextureView.hpp
            include/dvk/Sampler.hpp
)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
FetchContent_Declare(vmahpp
    URL      https://github.com/YaaZ/VulkanMemoryAllocator-Hpp/releases/download/v3.3.0%2B2/VulkanMemoryAllocator-Hpp-3.3.0.tar.gz
    URL_HASH SHA256=44767b5990dd36bdc3934b48a7bda2906a5573199a12b61ef2f0de8e72f988f1
)
FetchContent_MakeAvailable(vmahpp)

target_compile_definitions(dvk
    PUBLIC
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)

target_link_libraries(dvk
    PRIVATE
        glfw
        Vulkan::Vulkan
        glm::glm
        VulkanMemoryAllocator-Hpp
)

target_include_directories(dvk
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:include>
)
