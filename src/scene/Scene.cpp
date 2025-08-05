#include "Scene.hpp"
#include "Camera.hpp"
#include "Model.hpp"
#include "Light.hpp"
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace VulkanViewer {

Scene::Scene() {
    m_camera = std::make_unique<Camera>();
    setupDefaultLights();
}

Scene::~Scene() = default;

void Scene::update(float deltaTime) {

    if (m_camera) {

    }
    

    for (auto& model : m_models) {

    }
}

void Scene::loadModel(const std::string& filepath) {
    auto model = std::make_unique<Model>();

}

void Scene::addModel(std::unique_ptr<Model> model) {
   
    glm::mat4 currentTransform = model->getTransform();
    bool isIdentityTransform = (currentTransform == glm::mat4(1.0f));
    
    if (isIdentityTransform) {
        int modelCount = m_models.size();
        float spacing = 5.0f;
        

        int gridSize = 3; 
        int row = modelCount / gridSize;
        int col = modelCount % gridSize;
        
        glm::vec3 position;
        position.x = (col - gridSize/2) * spacing;
        position.y = 0.0f; 
        position.z = (row - gridSize/2) * spacing;
        

        glm::mat4 transform = glm::mat4(1.0f);
        transform = glm::translate(transform, position); 
        transform = glm::scale(transform, glm::vec3(0.1f, 0.1f, 0.1f)); 
        
        model->setTransform(transform);
        
        std::cout << "Auto-positioned new model " << modelCount << " at (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
    } else {
        std::cout << "Model already has custom transform, keeping it unchanged" << std::endl;
    }
    
    std::cout << "Scene: Adding MODEL " << (void*)model.get() << " to scene (total: " << m_models.size() + 1 << ")" << std::endl;
    m_models.push_back(std::move(model));
}

void Scene::removeModel(int index) {
    if (index >= 0 && index < static_cast<int>(m_models.size())) {
        m_models.erase(m_models.begin() + index);
    }
}

void Scene::clearModels() {
    m_models.clear();
}

void Scene::setupDefaultLights() {

    auto directionalLight = std::make_unique<Light>(LightType::Directional);
    directionalLight->setDirection(glm::vec3(-0.5f, -1.0f, -0.5f));
    directionalLight->setColor(glm::vec3(1.0f, 1.0f, 0.9f));
    directionalLight->setIntensity(1.0f);
    m_lights.push_back(std::move(directionalLight));
}

}