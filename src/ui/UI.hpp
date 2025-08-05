#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <memory>
#include <glm/glm.hpp>

struct GLFWwindow;

namespace VulkanViewer {

class VulkanDevice;
class Renderer;
class Scene;
class Model;

class UI {
public:
    UI(VulkanDevice& device, GLFWwindow* window, Renderer& renderer);
    ~UI();

    void render(Scene& scene);
    
    int getSelectedModelIndex() const { return m_selectedModelIndex; }
    void addLoadedModel(std::unique_ptr<Model> model);

private:
    void initImGui();
    void cleanup();
    
    void renderMainMenuBar(Scene& scene);
    void renderSceneHierarchy(Scene& scene);
    void renderStatistics();
    void renderAssetBrowser(Scene& scene);
    void renderProperties(Scene& scene);
    void renderSceneViewport(Scene& scene);
    
    std::string openFileDialog();
    void openFileDialog(Scene& scene);
    void loadModelToAssetBrowser();
    
    std::string openTextureFileDialog();
    void loadTextureForMesh(class Model* model, size_t meshIndex, const std::string& textureType);

    VulkanDevice& m_device;
    GLFWwindow* m_window;
    Renderer& m_renderer;
    
    VkDescriptorPool m_descriptorPool;
    
    float m_frameRate = 0.0f;
    int m_triangleCount = 0;
    int m_drawCalls = 0;
    

    int m_selectedModelIndex = -1;
    

    glm::vec3 m_editPosition = glm::vec3(0.0f);
    glm::vec3 m_editRotation = glm::vec3(0.0f);
    glm::vec3 m_editScale = glm::vec3(1.0f);
    bool m_transformInitialized = false;
    

    std::vector<std::unique_ptr<Model>> m_loadedModels;
};

}