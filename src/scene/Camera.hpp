#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace VulkanViewer {

enum class CameraMode {
    Arcball,
    FPS
};

class Camera {
public:
    Camera(float fov = 45.0f, float aspectRatio = 16.0f/9.0f, float nearPlane = 0.1f, float farPlane = 100.0f);

    void update(float deltaTime);
    
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
    glm::vec3 getPosition() const { return m_position; }
    glm::vec3 getDirection() const { return m_front; }
    
    void setPosition(const glm::vec3& position) { m_position = position; }
    void setTarget(const glm::vec3& target);
    void setMode(CameraMode mode) { m_mode = mode; }
    void setAspectRatio(float aspectRatio) { m_aspectRatio = aspectRatio; }
    void setMovementSpeed(float speed) { m_movementSpeed = speed; }
    void adjustFOV(float delta);
    void processFOVInput(bool zoomIn, bool zoomOut, float deltaTime);
    

    void orbitHorizontal(float angle);
    void orbitVertical(float angle);
    void zoom(float delta);
    void pan(float deltaX, float deltaY);
    

    void processMovement(float deltaTime, bool forward, bool backward, bool left, bool right, bool up, bool down);
    void processMouseMovement(float xoffset, float yoffset);

    void frameTarget(const glm::vec3& center, float radius);
    
    float getFOV() const { return m_fov; }
    float getMovementSpeed() const { return m_movementSpeed; }

private:
    void updateVectors();

    CameraMode m_mode = CameraMode::FPS;

    glm::vec3 m_position = glm::vec3(0.0f, 2.0f, 5.0f);  
    glm::vec3 m_front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 m_up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 m_right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 m_worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    

    float m_fov;
    float m_aspectRatio;
    float m_nearPlane;
    float m_farPlane;
    

    glm::vec3 m_target = glm::vec3(0.0f);
    float m_distance = 3.0f;
    float m_theta = 0.0f;      
    float m_phi = 0.0f;       
    

    float m_yaw = -90.0f;  
    float m_pitch = -15.0f;  
    float m_movementSpeed = 0.5f;
    float m_mouseSensitivity = 0.15f;
    

    float m_smoothingFactor = 0.1f;
    glm::vec3 m_targetPosition = glm::vec3(0.0f);
    bool m_smoothMovement = true;
    

    float m_rotationMomentumX = 0.0f;
    float m_rotationMomentumY = 0.0f;
    float m_momentumDecay = 0.85f;  
    

    glm::vec3 m_movementMomentum = glm::vec3(0.0f);
    float m_movementDecay = 0.90f;  
    

    float m_fovMomentum = 0.0f;
    float m_fovDecay = 0.88f;  
    float m_fovAcceleration = 120.0f;  
};

}