#include <DVK/Surface.hpp>
#include "DVK/Instance.hpp"

using namespace DVK;

Surface::Surface(const Instance& instance, VkSurfaceKHR surface_) {
    surface = vk::raii::SurfaceKHR(instance.GetInstance(), surface_);
}

const vk::raii::SurfaceKHR& Surface::GetSurface() const {
    return surface;
}
