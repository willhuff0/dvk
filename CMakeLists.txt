cmake_minimum_required(VERSION 4.1)
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
)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)

target_compile_definitions(dvk
    PUBLIC
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)

target_link_libraries(dvk
    PRIVATE
        glfw
        Vulkan::Vulkan
        glm::glm
)

target_include_directories(dvk
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:include>
)
