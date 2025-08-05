#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <memory>
#include <vector>
#include <string>


namespace VulkanViewer {
    class Model;
}

namespace VulkanViewer {

class VulkanDevice;
class Renderer;
class UI;
class Scene;
class Model;

class Application {
public:
    Application();
    ~Application();

    void run();

private:
    void initWindow();
    void initVulkan();
    void initImGui();
    void mainLoop();
    void cleanup();

    void processInput(float deltaTime);
    void update(float deltaTime);
    void render();

    static void framebuffer_resize_callback(GLFWwindow* window, int width, int height);
    static void drop_callback(GLFWwindow* window, int count, const char** paths);
    static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

    GLFWwindow* m_window = nullptr;
    uint32_t m_windowWidth = 1280;
    uint32_t m_windowHeight = 720;
    bool m_framebufferResized = false;

    std::unique_ptr<VulkanDevice> m_device;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<UI> m_ui;
    std::unique_ptr<Scene> m_scene;

    float m_lastFrameTime = 0.0f;
    

    bool m_rightMousePressed = false;
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;
    

    bool m_keys[1024] = {false};
    bool m_keysPressed[1024] = {false}; 
    float m_cameraSpeed = 0.5f;
    

    int m_selectedModelIndex = -1;
    Model* m_copiedModel = nullptr;
};

}