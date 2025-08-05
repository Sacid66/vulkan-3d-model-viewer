#include "Application.hpp"
#include "VulkanDevice.hpp"
#include "../rendering/Renderer.hpp"
#include "../ui/UI.hpp"
#include "../scene/Scene.hpp"
#include "../scene/Camera.hpp"
#include "../scene/Model.hpp"

#include <iostream>
#include <stdexcept>
#include <chrono>
#include <algorithm>

namespace VulkanViewer {

Application::Application() {
    initWindow();
    initVulkan();
    initImGui();
}

Application::~Application() {
    cleanup();
}

void Application::run() {
    mainLoop();
}

void Application::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_window = glfwCreateWindow(m_windowWidth, m_windowHeight, "Vulkan 3D Model Viewer", nullptr, nullptr);
    if (!m_window) {
        throw std::runtime_error("Failed to create GLFW window!");
    }

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebuffer_resize_callback);
    glfwSetDropCallback(m_window, drop_callback);
    glfwSetCursorPosCallback(m_window, mouse_callback);
    glfwSetMouseButtonCallback(m_window, mouse_button_callback);
    glfwSetScrollCallback(m_window, scroll_callback);
    glfwSetKeyCallback(m_window, key_callback);
}

void Application::initVulkan() {
    m_device = std::make_unique<VulkanDevice>(m_window);
    m_renderer = std::make_unique<Renderer>(*m_device, m_windowWidth, m_windowHeight);
    m_scene = std::make_unique<Scene>();
}

void Application::initImGui() {
    m_ui = std::make_unique<UI>(*m_device, m_window, *m_renderer);
}

void Application::mainLoop() {
    auto startTime = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(m_window)) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        float deltaTime = time - m_lastFrameTime;
        m_lastFrameTime = time;

        glfwPollEvents();
        
        processInput(deltaTime);
        update(deltaTime);
        render();
    }

    m_device->waitIdle();
}

void Application::processInput(float deltaTime) {
    if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(m_window, true);
    }
    
   
    if (m_rightMousePressed) {
        bool forward = m_keys[GLFW_KEY_W];
        bool backward = m_keys[GLFW_KEY_S];
        bool left = m_keys[GLFW_KEY_A];
        bool right = m_keys[GLFW_KEY_D];
        bool up = m_keys[GLFW_KEY_E];
        bool down = m_keys[GLFW_KEY_Q];
        

        bool zoomIn = m_keys[GLFW_KEY_Z];
        bool zoomOut = m_keys[GLFW_KEY_C];
        m_scene->getCamera().processFOVInput(zoomIn, zoomOut, deltaTime);
        

        m_scene->getCamera().processMovement(deltaTime, forward, backward, left, right, up, down);
    }
    

    bool ctrlPressed = glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
                      glfwGetKey(m_window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    
    if (ctrlPressed) {

        if (m_keysPressed[GLFW_KEY_C] && m_selectedModelIndex >= 0) {
            const auto& models = m_scene->getModels();
            if (m_selectedModelIndex < static_cast<int>(models.size())) {

                m_copiedModel = models[m_selectedModelIndex].get();
                std::cout << "Model copied to clipboard" << std::endl;
            }
        }
        

        if (m_keysPressed[GLFW_KEY_V] && m_copiedModel) {
            auto newModel = std::make_unique<Model>();
            if (newModel->copyFrom(*m_copiedModel, *m_device)) {

                newModel->setTransform(m_copiedModel->getTransform());
                m_scene->addModel(std::move(newModel));
                std::cout << "Model pasted from clipboard" << std::endl;
            }
        }
    }
    

    for (int i = 0; i < 1024; i++) {
        m_keysPressed[i] = false;
    }
}

void Application::update(float deltaTime) {

    m_scene->getCamera().update(deltaTime);
    m_scene->update(deltaTime);
    

    m_selectedModelIndex = m_ui->getSelectedModelIndex();
}

void Application::render() {
    if (m_framebufferResized) {
        int width = 0, height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(m_window, &width, &height);
            glfwWaitEvents();
        }

        m_windowWidth = width;
        m_windowHeight = height;
        m_renderer->recreateSwapChain(width, height);
        m_framebufferResized = false;
    }

    m_renderer->beginFrame();
    

    m_renderer->renderScene(*m_scene);
    

    m_ui->render(*m_scene);
    
    m_renderer->endFrame();
}

void Application::cleanup() {
    if (m_device) {
        m_device->waitIdle();
    }

    m_ui.reset();
    m_renderer.reset();
    m_scene.reset();
    m_device.reset();

    if (m_window) {
        glfwDestroyWindow(m_window);
    }
    glfwTerminate();
}

void Application::framebuffer_resize_callback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app->m_framebufferResized = true;
}

void Application::drop_callback(GLFWwindow* window, int count, const char** paths) {
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    
    for (int i = 0; i < count; i++) {
        std::string filepath(paths[i]);
        std::cout << "Dropped file: " << filepath << std::endl;
        

        std::string extension = filepath.substr(filepath.find_last_of('.'));
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        

        std::vector<std::string> supportedFormats = {
            ".obj", ".fbx", ".dae", ".gltf", ".glb", ".blend", ".3ds", ".ase", ".ifc",
            ".xgl", ".zgl", ".ply", ".dxf", ".lwo", ".lws", ".lxo", ".stl", ".x", ".ac",
            ".ms3d", ".cob", ".scn", ".bvh", ".csm", ".xml", ".irrmesh", ".irr", ".mdl",
            ".md2", ".md3", ".pk3", ".mdc", ".md5", ".smd", ".vta", ".ogex", ".3d", ".b3d",
            ".q3d", ".q3s", ".nff", ".nff", ".off", ".raw", ".ter", ".hmp", ".ndo"
        };
        
        bool isSupported = std::find(supportedFormats.begin(), supportedFormats.end(), extension) != supportedFormats.end();
        
        if (isSupported) {
            auto model = std::make_unique<Model>();
            if (model->loadFromFile(filepath, *app->m_device)) {

                app->m_ui->addLoadedModel(std::move(model));
                std::cout << "Successfully loaded model to asset browser: " << filepath << std::endl;
            } else {
                std::cerr << "Failed to load model: " << filepath << std::endl;
            }
        } else {
            std::cout << "Unsupported file format: " << extension << std::endl;
            std::cout << "Supported formats: OBJ, FBX, DAE, GLTF, GLB, BLEND, 3DS, PLY, STL, and many more" << std::endl;
        }
    }
}

void Application::mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    

    if (app->m_rightMousePressed) {
        double xoffset = xpos - app->m_lastMouseX;
        double yoffset = app->m_lastMouseY - ypos; 
        

        

        float sensitivity = 1.0f;
        app->m_scene->getCamera().processMouseMovement(xoffset * sensitivity, yoffset * sensitivity);
    }
    
    app->m_lastMouseX = xpos;
    app->m_lastMouseY = ypos;
}

void Application::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {

            app->m_rightMousePressed = true;
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            app->m_lastMouseX = xpos;
            app->m_lastMouseY = ypos;

            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else if (action == GLFW_RELEASE) {

            app->m_rightMousePressed = false;

            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void Application::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    

    if (yoffset > 0) {

        app->m_cameraSpeed = std::min(10.0f, app->m_cameraSpeed * 1.2f);
    } else {

        app->m_cameraSpeed = std::max(0.1f, app->m_cameraSpeed * 0.8f);
    }
    app->m_scene->getCamera().setMovementSpeed(app->m_cameraSpeed);
    std::cout << "Camera speed: " << app->m_cameraSpeed << std::endl;
}

void Application::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            app->m_keys[key] = true;
            app->m_keysPressed[key] = true;  
        } else if (action == GLFW_RELEASE) {
            app->m_keys[key] = false;
        }
    }
}

}