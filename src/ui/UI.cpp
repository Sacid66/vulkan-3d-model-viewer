#include "UI.hpp"
#include "../core/VulkanDevice.hpp"
#include "../rendering/Renderer.hpp"
#include "../rendering/ThumbnailRenderer.hpp"
#include "../scene/Scene.hpp"
#include "../scene/Camera.hpp"
#include "../scene/Model.hpp"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <stdexcept>
#include <array>
#include <iostream>
#include <filesystem>
#include <windows.h>
#include <commdlg.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace VulkanViewer {

UI::UI(VulkanDevice& device, GLFWwindow* window, Renderer& renderer) 
    : m_device(device), m_window(window), m_renderer(renderer) {
    initImGui();
}

UI::~UI() {
    cleanup();
}

void UI::render(Scene& scene) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered() && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
        m_selectedModelIndex = -1;
        m_transformInitialized = false;
    }
    
    renderMainMenuBar(scene);
    renderSceneHierarchy(scene);
    renderStatistics();
    renderProperties(scene);
    renderAssetBrowser(scene);
    renderSceneViewport(scene);
    
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_renderer.getCurrentCommandBuffer());
}

void UI::initImGui() {

    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    
    if (vkCreateDescriptorPool(m_device.getDevice(), &pool_info, nullptr, &m_descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create ImGui descriptor pool!");
    }
    

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;


    ImGui::StyleColorsDark();
    

    ImGui_ImplGlfw_InitForVulkan(m_window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_device.getInstance();
    init_info.PhysicalDevice = m_device.getPhysicalDevice();
    init_info.Device = m_device.getDevice();
    init_info.QueueFamily = m_device.getQueueFamilies().graphicsFamily.value();
    init_info.Queue = m_device.getGraphicsQueue();
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = m_descriptorPool;
    init_info.RenderPass = m_renderer.getRenderPass();
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = nullptr;
    
    ImGui_ImplVulkan_Init(&init_info);
    

}

void UI::cleanup() {
    if (m_descriptorPool) {
        vkDestroyDescriptorPool(m_device.getDevice(), m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }
    
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void UI::renderMainMenuBar(Scene& scene) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Import Model...")) {

                std::cout << "Drag & drop model files (OBJ, FBX, GLTF, DAE, BLEND, STL, etc.) into the window" << std::endl;
            }
            if (ImGui::MenuItem("Clear Scene")) {
                scene.clearModels();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit")) {
                glfwSetWindowShouldClose(m_window, true);
            }
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
}

void UI::renderSceneHierarchy(Scene& scene) {

    ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(250, 400), ImGuiCond_Always);
    
    ImGui::Begin("Scene Hierarchy", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    

    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    bool isDropTarget = false;
    

    ImVec2 contentSize = ImGui::GetContentRegionAvail();
    ImGui::Dummy(ImVec2(contentSize.x, contentSize.y));
    if (ImGui::BeginDragDropTarget()) {
        isDropTarget = true;
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_BROWSER_MODEL")) {
            std::cout << "PAYLOAD ACCEPTED on Scene Hierarchy content area!" << std::endl;
            size_t sourceIndex = *(const size_t*)payload->Data;
            
            if (sourceIndex < m_loadedModels.size()) {
                // Wait for any pending rendering to complete before modifying descriptor sets
                m_device.waitIdle();
                
                auto newModel = std::make_unique<Model>();
                if (newModel->copyFrom(*m_loadedModels[sourceIndex], m_device)) {
                    newModel->setTransform(glm::mat4(1.0f));
                    scene.addModel(std::move(newModel));
                    std::cout << "SUCCESS: Model added to scene via content drop" << std::endl;
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
    

    ImGui::SetCursorPos(ImVec2(8, 32)); 
    

    if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered()) {
        m_selectedModelIndex = -1;
        m_transformInitialized = false;
    }
    
    if (ImGui::CollapsingHeader("Models", ImGuiTreeNodeFlags_DefaultOpen)) {
        const auto& models = scene.getModels();
        

        if (m_selectedModelIndex >= static_cast<int>(models.size())) {
            m_selectedModelIndex = -1;
        }
        
        if (models.empty()) {
            ImGui::Text("No models loaded");
            ImGui::Text("Drag & drop model files:");
            ImGui::Text("OBJ, FBX, GLTF, DAE, BLEND, STL...");
        } else {
            for (size_t i = 0; i < models.size(); i++) {
                const std::string& modelName = models[i]->getName();
                bool isSelected = (m_selectedModelIndex == static_cast<int>(i));
                

                if (i > 0) ImGui::Spacing();
                

                std::string uniqueID = modelName + "##" + std::to_string(i);
                if (ImGui::Selectable(uniqueID.c_str(), isSelected)) {
                    if (m_selectedModelIndex != static_cast<int>(i)) {
                        m_selectedModelIndex = static_cast<int>(i);
                        m_transformInitialized = false; 
                    }
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Model: %s", modelName.c_str());
                    ImGui::Text("Meshes: %zu", models[i]->getMeshes().size());
                    ImGui::EndTooltip();
                }
            }
        }
    }
    

    if (ImGui::IsWindowHovered() && ImGui::GetDragDropPayload() != nullptr) {
        const ImGuiPayload* payload = ImGui::GetDragDropPayload();
        if (payload && strcmp(payload->DataType, "ASSET_BROWSER_MODEL") == 0) {
            isDropTarget = true;
            

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                std::cout << "PAYLOAD ACCEPTED on Scene Hierarchy full panel via window hover!" << std::endl;
                size_t sourceIndex = *(const size_t*)payload->Data;
                
                if (sourceIndex < m_loadedModels.size()) {
                    // Wait for any pending rendering to complete before modifying descriptor sets
                    m_device.waitIdle();
                    
                    auto newModel = std::make_unique<Model>();
                    if (newModel->copyFrom(*m_loadedModels[sourceIndex], m_device)) {
                        newModel->setTransform(glm::mat4(1.0f));
                        scene.addModel(std::move(newModel));
                        std::cout << "SUCCESS: Model added to scene via full panel drop" << std::endl;
                    }
                }
            }
        }
    }
    

    if (isDropTarget) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 windowMax = ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y);
        drawList->AddRect(windowPos, windowMax, IM_COL32(255, 255, 0, 255), 0.0f, 0, 3.0f); 
    }
    
    ImGui::End();
}

void UI::renderStatistics() {

    ImGuiIO& io = ImGui::GetIO();
    

    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 280, 30), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(270, 120), ImGuiCond_Always);
    
    ImGui::Begin("Statistics", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    
    ImGui::Text("FPS: %.1f", io.Framerate);
    ImGui::Text("Frame Time: %.3f ms", 1000.0f / io.Framerate);
    ImGui::Text("Triangles: %d", m_triangleCount);
    ImGui::Text("Draw Calls: %d", m_drawCalls);
    
    ImGui::End();
}

void UI::renderAssetBrowser(Scene& scene) {

    ImGuiIO& io = ImGui::GetIO();
    

    float panelHeight = 200.0f;
    ImGui::SetNextWindowPos(ImVec2(270, io.DisplaySize.y - panelHeight - 10), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x - 560, panelHeight), ImGuiCond_Always);
    
    ImGui::Begin("Asset Browser", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    

    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 120.0f);
    if (ImGui::Button("Import Model", ImVec2(110, 30))) {
        loadModelToAssetBrowser();
    }
    
    ImGui::Separator();
    

    ImGui::BeginChild("AssetList", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    

    if (m_loadedModels.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No assets loaded. Use 'Import Model' button or drag & drop files.");
    } else {
        ImGui::Text("Loaded Models (%zu):", m_loadedModels.size());
        ImGui::Separator();
        

        const float thumbnailSize = 100.0f; 
        const float itemSpacing = 10.0f;
        const float itemWidth = thumbnailSize + itemSpacing;
        const float availableWidth = ImGui::GetContentRegionAvail().x;
        const int itemsPerRow = std::max(1, (int)(availableWidth / itemWidth));
        
        ThumbnailRenderer* thumbnailRenderer = m_renderer.getThumbnailRenderer();
        
        for (size_t i = 0; i < m_loadedModels.size(); i++) {
            const std::string& modelName = m_loadedModels[i]->getName();
            
            ImGui::BeginGroup();
            

            if (thumbnailRenderer && !thumbnailRenderer->hasThumbnail(modelName)) {
                thumbnailRenderer->generateThumbnail(m_loadedModels[i].get(), modelName);
            }
            

            ThumbnailData* thumbnail = thumbnailRenderer ? thumbnailRenderer->getThumbnail(modelName) : nullptr;
            

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.4f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.6f, 1.0f));
            
            if (ImGui::Button(("##static_" + std::to_string(i)).c_str(), ImVec2(thumbnailSize, thumbnailSize))) {
                m_selectedModelIndex = static_cast<int>(i);
                m_transformInitialized = false;
            }
            

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {

                size_t dragIndex = i;  
                ImGui::SetDragDropPayload("ASSET_BROWSER_MODEL", &dragIndex, sizeof(size_t));
                
 
                ImGui::Text("Dragging: %s", modelName.c_str());
                std::cout << "Started dragging model: " << modelName << " (index: " << i << ") - Payload size: " << sizeof(size_t) << std::endl;
                ImGui::EndDragDropSource();
            }
            

            ImVec2 button_pos = ImGui::GetItemRectMin();
            ImVec2 button_size = ImGui::GetItemRectSize();
            ImVec2 text_size = ImGui::CalcTextSize("Static Mesh");
            ImVec2 text_pos = ImVec2(
                button_pos.x + (button_size.x - text_size.x) * 0.5f,
                button_pos.y + (button_size.y - text_size.y) * 0.5f
            );
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            draw_list->AddText(text_pos, IM_COL32(200, 200, 200, 255), "Static Mesh");
            
            ImGui::PopStyleColor(3);
            

            const float textWidth = ImGui::CalcTextSize(modelName.c_str()).x;
            const float textOffset = (thumbnailSize - textWidth) * 0.5f;
            if (textOffset > 0) {
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textOffset);
            }
            

            std::string displayName = modelName;
            if (displayName.length() > 15) {
                displayName = displayName.substr(0, 12) + "...";
            }
            
            ImGui::Text("%s", displayName.c_str());

            if (ImGui::IsItemHovered() || ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Model: %s", modelName.c_str());
                ImGui::Text("Meshes: %zu", m_loadedModels[i]->getMeshes().size());
                ImGui::Text("Materials: %zu", m_loadedModels[i]->getMaterials().size());
                ImGui::EndTooltip();
            }
            
            ImGui::EndGroup();
            

            if ((i + 1) % itemsPerRow != 0 && i < m_loadedModels.size() - 1) {
                ImGui::SameLine();
            }
        }
    }
    
    ImGui::EndChild();
    ImGui::End();
}

std::string UI::openFileDialog() {
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = glfwGetWin32Window(m_window);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "3D Model Files\0*.obj;*.fbx;*.dae;*.gltf;*.glb;*.blend;*.3ds;*.ase;*.ifc;*.xgl;*.zgl;*.ply;*.dxf;*.lwo;*.lws;*.lxo;*.stl;*.x;*.ac;*.ms3d;*.cob;*.scn;*.md2;*.md3;*.pk3;*.mdc;*.md5mesh;*.smd;*.vta;*.ogex;*.3mf;*.b3d;*.q3d;*.q3s;*.nff;*.nendo;*.ter;*.mdl;*.hmp;*.mesh.xml;*.irrmesh;*.irr;*.pmx;*.prj\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(szFile);
    }
    
    return "";
}

void UI::openFileDialog(Scene& scene) {
    std::string filepath = openFileDialog();
    if (!filepath.empty()) {

        auto model = std::make_unique<Model>();
        if (model->loadFromFile(filepath, m_device)) {

            model->setTransform(glm::mat4(1.0f)); 
            scene.addModel(std::move(model));
            std::cout << "Successfully loaded model: " << filepath << std::endl;
        } else {
            std::cerr << "Failed to load model: " << filepath << std::endl;
        }
    }
}

void UI::loadModelToAssetBrowser() {
    std::string filepath = openFileDialog();
    if (!filepath.empty()) {

        auto model = std::make_unique<Model>();
        if (model->loadFromFile(filepath, m_device)) {
            m_loadedModels.push_back(std::move(model));
            std::cout << "Successfully loaded model to asset browser: " << filepath << std::endl;
        } else {
            std::cerr << "Failed to load model: " << filepath << std::endl;
        }
    }
}

void UI::addLoadedModel(std::unique_ptr<Model> model) {
    m_loadedModels.push_back(std::move(model));
}

void UI::renderProperties(Scene& scene) {

    ImGuiIO& io = ImGui::GetIO();
    

    float statisticsHeight = 120.0f;
    float panelTop = 30 + statisticsHeight + 10;
    float panelHeight = io.DisplaySize.y - panelTop - 220; 
    
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - 280, panelTop), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(270, panelHeight), ImGuiCond_Always);
    
    ImGui::Begin("Properties", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    
    const auto& models = scene.getModels();
    
    if (m_selectedModelIndex < 0 || m_selectedModelIndex >= static_cast<int>(models.size())) {

        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No object selected");
        ImGui::Separator();
        ImGui::Text("Select an object from the");
        ImGui::Text("Scene Hierarchy to view");
        ImGui::Text("its properties.");
    } else {

        auto& selectedModel = models[m_selectedModelIndex];
        
        ImGui::Text("Selected: %s", selectedModel->getName().c_str());
        ImGui::Separator();
        

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {

            if (!m_transformInitialized) {
                glm::mat4 transform = selectedModel->getTransform();
                

                m_editPosition = glm::vec3(transform[3][0], transform[3][1], transform[3][2]);
                

                m_editScale = glm::vec3(
                    glm::length(glm::vec3(transform[0])),
                    glm::length(glm::vec3(transform[1])),
                    glm::length(glm::vec3(transform[2]))
                );
                

                m_editRotation = glm::vec3(0.0f);
                
                m_transformInitialized = true;
            }
            
            bool transformChanged = false;
            
            ImGui::Text("Position:");
            if (ImGui::DragFloat3("##Position", glm::value_ptr(m_editPosition), 0.1f)) {
                transformChanged = true;
            }
            
            ImGui::Text("Rotation (degrees):");
            if (ImGui::DragFloat3("##Rotation", glm::value_ptr(m_editRotation), 1.0f)) {
                transformChanged = true;
            }
            
            ImGui::Text("Scale:");
            if (ImGui::DragFloat3("##Scale", glm::value_ptr(m_editScale), 0.01f)) {
                transformChanged = true;
            }
            

            if (transformChanged) {
                glm::mat4 newTransform = glm::mat4(1.0f);
                newTransform = glm::translate(newTransform, m_editPosition);

                newTransform = glm::rotate(newTransform, glm::radians(m_editRotation.x), glm::vec3(1, 0, 0));
                newTransform = glm::rotate(newTransform, glm::radians(m_editRotation.y), glm::vec3(0, 1, 0));
                newTransform = glm::rotate(newTransform, glm::radians(m_editRotation.z), glm::vec3(0, 0, 1));
                
                newTransform = glm::scale(newTransform, m_editScale);
                
                selectedModel->setTransform(newTransform);
                

                std::cout << "UI: MODEL " << (void*)selectedModel.get() << " moved to (" 
                          << m_editPosition.x << ", " << m_editPosition.y << ", " << m_editPosition.z << ")" << std::endl;
            }
        }
        

        if (ImGui::CollapsingHeader("Model Info", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("Meshes: %zu", selectedModel->getMeshes().size());
            ImGui::Text("Materials: %zu", selectedModel->getMaterials().size());
            

            const auto& meshes = selectedModel->getMeshes();
            for (size_t i = 0; i < meshes.size(); i++) {
                ImGui::Text("Mesh %zu:", i);
                ImGui::Indent();
                ImGui::Text("  Vertices: %zu", meshes[i].vertices.size());
                ImGui::Text("  Indices: %zu", meshes[i].indices.size());
                ImGui::Unindent();
            }
        }
        

        if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
            const auto& meshes = selectedModel->getMeshes();
            const auto& materials = selectedModel->getMaterials();
            
            ImGui::Text("Texture assignment per mesh:");
            ImGui::Separator();
            
            for (size_t i = 0; i < meshes.size(); i++) {
                ImGui::Text("Mesh %zu:", i);
                ImGui::Indent();
                

                if (i < materials.size()) {
                    const auto& material = materials[i];
                    ImGui::Text("Material: %s", material.name.c_str());
                    

                    if (!material.diffuseTexture.empty()) {
                        ImGui::Text("Base Color: %s", material.diffuseTexture.c_str());
                    } else {
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Base Color: Not assigned");
                    }
                    
                    if (ImGui::Button(("Load Base Color##" + std::to_string(i)).c_str(), ImVec2(120, 0))) {
                        loadTextureForMesh(selectedModel.get(), i, "diffuse");
                    }
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No material assigned");
                }
                
                ImGui::Unindent();
                ImGui::Separator();
            }
        }
        

        if (ImGui::CollapsingHeader("UV Controls (Advanced)")) {
            bool forceUVFlip = selectedModel->getForceUVFlip();
            if (ImGui::Checkbox("Force UV Flip Override", &forceUVFlip)) {
                selectedModel->setForceUVFlip(forceUVFlip, m_device);
                std::cout << "UV Flip override " << (forceUVFlip ? "ENABLED" : "DISABLED") 
                          << " for model: " << selectedModel->getName() << std::endl;
            }
            
            ImGui::TextWrapped("Advanced: Manual override for UV coordinates. The engine automatically detects and fixes UV mapping issues.");
            ImGui::TextWrapped("Only use this if automatic detection fails.");
        }
        

        if (ImGui::CollapsingHeader("Actions")) {
            if (ImGui::Button("Reset Transform", ImVec2(-1, 0))) {
                selectedModel->setTransform(glm::mat4(1.0f));

                m_editPosition = glm::vec3(0.0f);
                m_editRotation = glm::vec3(0.0f);
                m_editScale = glm::vec3(1.0f);
                m_transformInitialized = true;
            }
            
            if (ImGui::Button("Remove from Scene", ImVec2(-1, 0))) {
                scene.removeModel(m_selectedModelIndex);
                m_selectedModelIndex = -1;
                m_transformInitialized = false;
            }
        }
    }
    
    ImGui::End();
}

std::string UI::openTextureFileDialog() {
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = glfwGetWin32Window(m_window);
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga;*.dds;*.hdr;*.pic;*.ppm;*.pgm\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    
    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(szFile);
    }
    
    return "";
}

void UI::loadTextureForMesh(Model* model, size_t meshIndex, const std::string& textureType) {
    std::string filepath = openTextureFileDialog();
    if (!filepath.empty()) {

        model->setMaterialTexture(meshIndex, textureType, filepath);
        

        auto& materials = model->getMaterials();
        if (meshIndex < materials.size()) {
            if (model->loadTextureToGPU(materials[meshIndex], filepath, m_device)) {
                std::cout << "Successfully loaded " << textureType << " texture to GPU for material " << meshIndex << std::endl;
                

            } else {
                std::cerr << "Failed to load " << textureType << " texture to GPU for material " << meshIndex << std::endl;
            }
        }
    }
}

void UI::renderSceneViewport(Scene& scene) {
   
    ImGuiIO& io = ImGui::GetIO();
    

    float leftPanelWidth = 270.0f;  
    float rightPanelWidth = 280.0f; 
    float topMenuHeight = 30.0f;    
    float bottomPanelHeight = 200.0f; 
    
    float viewportX = leftPanelWidth;
    float viewportY = topMenuHeight;
    float viewportWidth = io.DisplaySize.x - leftPanelWidth - rightPanelWidth;
    float viewportHeight = io.DisplaySize.y - topMenuHeight - bottomPanelHeight;
    
    ImGui::SetNextWindowPos(ImVec2(viewportX, viewportY));
    ImGui::SetNextWindowSize(ImVec2(viewportWidth, viewportHeight));
    

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
    
    ImGui::Begin("##SceneViewport", nullptr, 
        ImGuiWindowFlags_NoTitleBar | 
        ImGuiWindowFlags_NoResize | 
        ImGuiWindowFlags_NoMove | 
        ImGuiWindowFlags_NoCollapse | 
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoFocusOnAppearing);

    

    if (ImGui::BeginDragDropTarget()) {
        std::cout << "Viewport accepting drag drop!" << std::endl;
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_BROWSER_MODEL")) {
            size_t sourceIndex = *(const size_t*)payload->Data;
            std::cout << "Received drop payload, index: " << sourceIndex << ", loaded models count: " << m_loadedModels.size() << std::endl;
            
            if (sourceIndex < m_loadedModels.size()) {
                // Wait for any pending rendering to complete before modifying descriptor sets
                m_device.waitIdle();

                auto newModel = std::make_unique<Model>();
                std::cout << "Attempting to copy model: " << m_loadedModels[sourceIndex]->getName() << std::endl;
                if (newModel->copyFrom(*m_loadedModels[sourceIndex], m_device)) {

                    newModel->setTransform(glm::mat4(1.0f)); 
                    scene.addModel(std::move(newModel));
                    std::cout << "SUCCESS: Model added to scene via drag and drop from asset browser" << std::endl;
                } else {
                    std::cout << "ERROR: Failed to copy model" << std::endl;
                }
            } else {
                std::cout << "ERROR: Invalid source index" << std::endl;
            }
        }
        ImGui::EndDragDropTarget();
    }
    
    ImGui::End();
    ImGui::PopStyleColor(2);
}

}