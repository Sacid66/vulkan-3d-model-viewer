#include "Light.hpp"

namespace VulkanViewer {

Light::Light(LightType type) : m_type(type) {
}

Light::~Light() = default;

}