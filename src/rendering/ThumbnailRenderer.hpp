#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

namespace VulkanViewer {

class VulkanDevice;
class Model;

struct ThumbnailData {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory imageMemory = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    uint32_t width = 512;
    uint32_t height = 512;
    bool isGenerated = false;
};

class ThumbnailRenderer {
public:
    ThumbnailRenderer(VulkanDevice& device, class Renderer& mainRenderer);
    ~ThumbnailRenderer();
    

    void updateDescriptorSetWithDefaultTexture();


    bool generateThumbnail(const Model* model, const std::string& modelName);
    

    ThumbnailData* getThumbnail(const std::string& modelName);
    

    bool hasThumbnail(const std::string& modelName) const;
    

    void clearThumbnails();

private:
    void createOffscreenResources();
    void cleanupOffscreenResources();
    void createModelPipeline();
    void renderModelToTexture(const Model* model, VkImage targetImage);
    void createThumbnailTexture(const std::string& modelName);
    std::vector<char> readFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    
    VulkanDevice& m_device;
    class Renderer& m_mainRenderer;
    

    VkRenderPass m_offscreenRenderPass = VK_NULL_HANDLE;
    VkFramebuffer m_offscreenFramebuffer = VK_NULL_HANDLE;
    

    VkImage m_colorImage = VK_NULL_HANDLE;
    VkDeviceMemory m_colorImageMemory = VK_NULL_HANDLE;
    VkImageView m_colorImageView = VK_NULL_HANDLE;
    

    VkImage m_depthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
    VkImageView m_depthImageView = VK_NULL_HANDLE;
    

    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    

    VkPipeline m_modelPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_modelPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
    

    VkBuffer m_uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_uniformBufferMemory = VK_NULL_HANDLE;
    void* m_uniformBufferMapped = nullptr;
    

    std::unordered_map<std::string, std::unique_ptr<ThumbnailData>> m_thumbnails;
    
    static const uint32_t THUMBNAIL_SIZE = 1024;
};

}