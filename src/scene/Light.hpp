#pragma once

#include <glm/glm.hpp>

namespace VulkanViewer {

enum class LightType {
    Directional,
    Point,
    Spot
};

class Light {
public:
    Light(LightType type = LightType::Directional);
    ~Light();
    
    LightType getType() const { return m_type; }
    void setType(LightType type) { m_type = type; }
    
    glm::vec3 getPosition() const { return m_position; }
    void setPosition(const glm::vec3& position) { m_position = position; }
    
    glm::vec3 getDirection() const { return m_direction; }
    void setDirection(const glm::vec3& direction) { m_direction = direction; }
    
    glm::vec3 getColor() const { return m_color; }
    void setColor(const glm::vec3& color) { m_color = color; }
    
    float getIntensity() const { return m_intensity; }
    void setIntensity(float intensity) { m_intensity = intensity; }

private:
    LightType m_type;
    glm::vec3 m_position = glm::vec3(0.0f, 5.0f, 0.0f);
    glm::vec3 m_direction = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 m_color = glm::vec3(1.0f, 1.0f, 1.0f);
    float m_intensity = 1.0f;
};

}