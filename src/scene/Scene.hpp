#pragma once

#include <vector>
#include <memory>
#include <string>

namespace VulkanViewer {

class Model;
class Camera;
class Light;

class Scene {
public:
    Scene();
    ~Scene();

    void update(float deltaTime);
    
    void loadModel(const std::string& filepath);
    void addModel(std::unique_ptr<Model> model);
    void removeModel(int index);
    void clearModels();
    
    Camera& getCamera() { return *m_camera; }
    const Camera& getCamera() const { return *m_camera; }
    
    const std::vector<std::unique_ptr<Model>>& getModels() const { return m_models; }
    const std::vector<std::unique_ptr<Light>>& getLights() const { return m_lights; }

private:
    void setupDefaultLights();

    std::vector<std::unique_ptr<Model>> m_models;
    std::vector<std::unique_ptr<Light>> m_lights;
    std::unique_ptr<Camera> m_camera;
};

}