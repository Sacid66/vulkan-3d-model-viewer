#include "Camera.hpp"
#include <algorithm>
#include <iostream>

namespace VulkanViewer {

Camera::Camera(float fov, float aspectRatio, float nearPlane, float farPlane) 
    : m_fov(fov), m_aspectRatio(aspectRatio), m_nearPlane(nearPlane), m_farPlane(farPlane) {
    m_targetPosition = m_position;
    updateVectors();
}

void Camera::update(float deltaTime) {

    if (m_smoothMovement && m_mode == CameraMode::Arcball) {
        glm::vec3 positionDiff = m_targetPosition - m_position;
        if (glm::length(positionDiff) > 0.001f) {
            m_position += positionDiff * m_smoothingFactor;
        }
    }
    

    if (m_mode == CameraMode::FPS) {

        if (abs(m_rotationMomentumX) > 0.01f || abs(m_rotationMomentumY) > 0.01f) {
            m_yaw += m_rotationMomentumX;
            m_pitch += m_rotationMomentumY;
            

            m_pitch = std::max(-89.0f, std::min(89.0f, m_pitch));

            m_rotationMomentumX *= m_momentumDecay;
            m_rotationMomentumY *= m_momentumDecay;
            

            updateVectors();
        }
        

        if (glm::length(m_movementMomentum) > 0.001f) {
            m_position += m_movementMomentum * deltaTime;
            

            m_movementMomentum *= m_movementDecay;
        }
        

        if (abs(m_fovMomentum) > 0.01f) {
            m_fov += m_fovMomentum * deltaTime;
            m_fov = std::max(10.0f, std::min(120.0f, m_fov));
            

            m_fovMomentum *= m_fovDecay;
        }
    }
}

glm::mat4 Camera::getViewMatrix() const {
    if (m_mode == CameraMode::Arcball) {
        return glm::lookAt(m_position, m_target, m_up);
    } else {
        return glm::lookAt(m_position, m_position + m_front, m_up);
    }
}

glm::mat4 Camera::getProjectionMatrix() const {
    glm::mat4 proj = glm::perspective(glm::radians(m_fov), m_aspectRatio, m_nearPlane, m_farPlane);

    proj[1][1] *= -1;
    return proj;
}

void Camera::setTarget(const glm::vec3& target) {
    m_target = target;
    if (m_mode == CameraMode::Arcball) {
        updateVectors();
    }
}

void Camera::orbitHorizontal(float angle) {
    if (m_mode != CameraMode::Arcball) return;
    
    m_theta += angle;
    

    glm::vec3 newPosition;
    newPosition.x = m_target.x + m_distance * cos(m_phi) * cos(m_theta);
    newPosition.y = m_target.y + m_distance * sin(m_phi);
    newPosition.z = m_target.z + m_distance * cos(m_phi) * sin(m_theta);
    
    if (m_smoothMovement) {
        m_targetPosition = newPosition;
    } else {
        m_position = newPosition;
    }
    
    updateVectors();
}

void Camera::orbitVertical(float angle) {
    if (m_mode != CameraMode::Arcball) return;
    
    m_phi += angle;
    

    m_phi = std::max(-1.5f, std::min(1.5f, m_phi));
    

    glm::vec3 newPosition;
    newPosition.x = m_target.x + m_distance * cos(m_phi) * cos(m_theta);
    newPosition.y = m_target.y + m_distance * sin(m_phi);
    newPosition.z = m_target.z + m_distance * cos(m_phi) * sin(m_theta);
    
    if (m_smoothMovement) {
        m_targetPosition = newPosition;
    } else {
        m_position = newPosition;
    }
    
    updateVectors();
}

void Camera::zoom(float delta) {
    if (m_mode == CameraMode::Arcball) {
        m_distance -= delta;
        m_distance = std::max(0.1f, m_distance);
        

        m_position.x = m_target.x + m_distance * cos(m_phi) * cos(m_theta);
        m_position.y = m_target.y + m_distance * sin(m_phi);
        m_position.z = m_target.z + m_distance * cos(m_phi) * sin(m_theta);
    } else {

        m_position += m_front * delta;
    }
    updateVectors();
}

void Camera::pan(float deltaX, float deltaY) {
    if (m_mode == CameraMode::Arcball) {

        glm::vec3 right = glm::normalize(glm::cross(m_front, m_up));
        glm::vec3 up = glm::normalize(glm::cross(right, m_front));
        
        float panSpeed = m_distance * 0.001f; 
        m_target += right * deltaX * panSpeed;
        m_target += up * deltaY * panSpeed;
        

        m_position.x = m_target.x + m_distance * cos(m_phi) * cos(m_theta);
        m_position.y = m_target.y + m_distance * sin(m_phi);
        m_position.z = m_target.z + m_distance * cos(m_phi) * sin(m_theta);
        
        updateVectors();
    }
}

void Camera::processMovement(float deltaTime, bool forward, bool backward, bool left, bool right, bool up, bool down) {
    if (m_mode != CameraMode::FPS) return;
    
    float acceleration = m_movementSpeed * 8.0f;  

    glm::vec3 desiredMovement(0.0f);
    
    if (forward) desiredMovement += m_front;
    if (backward) desiredMovement -= m_front;
    if (left) desiredMovement -= m_right;
    if (right) desiredMovement += m_right;
    if (up) desiredMovement += m_up;
    if (down) desiredMovement -= m_up;
    

    if (glm::length(desiredMovement) > 0.0f) {
        desiredMovement = glm::normalize(desiredMovement);
    }
    

    m_movementMomentum += desiredMovement * acceleration * deltaTime;
    

    float maxSpeed = m_movementSpeed * 3.0f;
    if (glm::length(m_movementMomentum) > maxSpeed) {
        m_movementMomentum = glm::normalize(m_movementMomentum) * maxSpeed;
    }
}

void Camera::processMouseMovement(float xoffset, float yoffset) {
    if (m_mode != CameraMode::FPS) return;
    
    xoffset *= m_mouseSensitivity;
    yoffset *= m_mouseSensitivity;
    

    m_rotationMomentumX += xoffset * 0.3f;  
    m_rotationMomentumY += yoffset * 0.3f;
    

    m_yaw += xoffset * 0.7f;  
    m_pitch += yoffset * 0.7f;
    

    m_pitch = std::max(-89.0f, std::min(89.0f, m_pitch));
    
    updateVectors();
}

void Camera::frameTarget(const glm::vec3& center, float radius) {
    m_target = center;
    m_distance = radius * 2.5f;

    m_theta = 0.0f;
    m_phi = 0.0f;
    

    m_position = m_target + glm::vec3(0.0f, 0.0f, m_distance);
    
    updateVectors();
}

void Camera::adjustFOV(float delta) {
    m_fov += delta;
    m_fov = std::max(10.0f, std::min(120.0f, m_fov)); 
}

void Camera::processFOVInput(bool zoomIn, bool zoomOut, float deltaTime) {
    float desiredFOVChange = 0.0f;
    
    if (zoomIn) {
        desiredFOVChange = -m_fovAcceleration; 
    } else if (zoomOut) {
        desiredFOVChange = m_fovAcceleration;   
    }
    

    m_fovMomentum += desiredFOVChange * deltaTime;
    

    float maxFOVSpeed = 150.0f;  
    m_fovMomentum = std::max(-maxFOVSpeed, std::min(maxFOVSpeed, m_fovMomentum));
}

void Camera::updateVectors() {
    if (m_mode == CameraMode::FPS) {

        glm::vec3 front;
        front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        front.y = sin(glm::radians(m_pitch));
        front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        m_front = glm::normalize(front);
        

        m_right = glm::normalize(glm::cross(m_front, m_worldUp));
        m_up = glm::normalize(glm::cross(m_right, m_front));
    } else {

        m_front = glm::normalize(m_target - m_position);
        m_right = glm::normalize(glm::cross(m_front, m_worldUp));
        m_up = glm::normalize(glm::cross(m_right, m_front));
    }
}

}