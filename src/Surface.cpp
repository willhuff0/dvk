#include <dvk/Surface.hpp>
#include "dvk/Instance.hpp"

using namespace dvk;

Surface::Surface(const Instance& instance, VkSurfaceKHR surface_) {
    surface = vk::raii::SurfaceKHR(instance.GetInstance(), surface_);
}

const vk::raii::SurfaceKHR& Surface::GetSurface() const {
    return surface;
}
