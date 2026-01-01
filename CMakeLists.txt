cmake_minimum_required(VERSION 4.1)
project(DVK)

set(CMAKE_CXX_STANDARD 20)

add_library(DVK)

target_sources(DVK
    PRIVATE
        src/Instance.cpp
        src/Surface.cpp
        src/Device.cpp
        src/Presenter.cpp
    PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            include/DVK/Instance.hpp
            include/DVK/Surface.hpp
            include/DVK/Device.hpp
            include/DVK/Presenter.hpp
)

find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)

target_compile_definitions(DVK
    PUBLIC
        VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)

target_link_libraries(DVK
    PRIVATE
        glfw
        Vulkan::Vulkan
        glm::glm
)

target_include_directories(DVK
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
           $<INSTALL_INTERFACE:include>
)
