#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <glm/glm.hpp>

namespace VulkanViewer {

class ThumbnailRenderer;

struct UniformBufferObject {
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

struct PushConstants {
    alignas(16) glm::mat4 model;
    alignas(16) glm::vec3 materialDiffuse;
    alignas(4) float hasTexture; 
};

class VulkanDevice;
class Scene;
class SwapChain;

class Renderer {
public:
    Renderer(VulkanDevice& device, uint32_t width, uint32_t height);
    ~Renderer();

    void recreateSwapChain(uint32_t width, uint32_t height);
    
    void beginFrame();
    void renderScene(const Scene& scene);
    void endFrame();

    VkRenderPass getRenderPass() const { return m_renderPass; }
    VkCommandBuffer getCurrentCommandBuffer() const { return m_commandBuffers[m_currentFrame]; }
    uint32_t getCurrentImageIndex() const { return m_imageIndex; }
    
    ThumbnailRenderer* getThumbnailRenderer() const { return m_thumbnailRenderer.get(); }
    

    VkImageView getDefaultTextureImageView() const { return m_defaultTextureImageView; }
    VkSampler getDefaultTextureSampler() const { return m_defaultTextureSampler; }

    static const int MAX_FRAMES_IN_FLIGHT = 2;

private:
    void createRenderPass();
    void createFramebuffers();
    void createCommandBuffers();
    void createSyncObjects();
    void createGridPipeline();
    void createModelPipeline();
    void createUniformBuffers();
    void updateUniformBuffer(uint32_t currentImage, const Scene& scene);
    void updateUniformBufferForModel(uint32_t currentImage, const Scene& scene, const class Model* model);
    void updateModelUniformBuffer(uint32_t currentImage, const Scene& scene, const class Model* model);
    void updateGridUniformBuffer(uint32_t currentImage, const Scene& scene);
    void createDefaultTexture();
    void updateDescriptorSetForMesh(const class Model* model, size_t materialIndex);
    std::vector<char> readFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void cleanup();

    VulkanDevice& m_device;
    std::unique_ptr<SwapChain> m_swapChain;

    VkRenderPass m_renderPass;
    std::vector<VkFramebuffer> m_framebuffers;
    
    std::vector<VkCommandBuffer> m_commandBuffers;
    
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    

    VkPipeline m_gridPipeline;
    VkPipelineLayout m_gridPipelineLayout;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_gridDescriptorSets;
    

    VkPipeline m_modelPipeline;
    VkPipelineLayout m_modelPipelineLayout;
    std::vector<VkDescriptorSet> m_modelDescriptorSets;
    

    std::vector<VkBuffer> m_modelUniformBuffers;
    std::vector<VkDeviceMemory> m_modelUniformBuffersMemory;
    std::vector<void*> m_modelUniformBuffersMapped;
    
    std::vector<VkBuffer> m_gridUniformBuffers;
    std::vector<VkDeviceMemory> m_gridUniformBuffersMemory;
    std::vector<void*> m_gridUniformBuffersMapped;
    

    VkImage m_defaultTextureImage = VK_NULL_HANDLE;
    VkDeviceMemory m_defaultTextureImageMemory = VK_NULL_HANDLE;
    VkImageView m_defaultTextureImageView = VK_NULL_HANDLE;
    VkSampler m_defaultTextureSampler = VK_NULL_HANDLE;
    

    std::unique_ptr<ThumbnailRenderer> m_thumbnailRenderer;
    
    size_t m_currentFrame = 0;
    uint32_t m_imageIndex = 0;
};

}