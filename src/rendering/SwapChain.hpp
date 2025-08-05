#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace VulkanViewer {

class VulkanDevice;

class SwapChain {
public:
    SwapChain(VulkanDevice& device, uint32_t width, uint32_t height);
    ~SwapChain();

    VkSwapchainKHR getSwapChain() const { return m_swapChain; }
    VkFormat getImageFormat() const { return m_swapChainImageFormat; }
    VkExtent2D getExtent() const { return m_swapChainExtent; }
    size_t getImageCount() const { return m_swapChainImages.size(); }
    VkImageView getImageView(size_t index) const { return m_swapChainImageViews[index]; }
    VkImageView getDepthImageView() const { return m_depthImageView; }

    VkResult acquireNextImage(VkSemaphore semaphore, uint32_t* imageIndex);
    VkResult present(VkQueue presentQueue, VkSemaphore* waitSemaphores, uint32_t imageIndex);

private:
    void createSwapChain(uint32_t width, uint32_t height);
    void createImageViews();
    void createDepthResources();
    void cleanup();

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);

    VulkanDevice& m_device;
    
    VkSwapchainKHR m_swapChain;
    std::vector<VkImage> m_swapChainImages;
    std::vector<VkImageView> m_swapChainImageViews;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;
    
    VkImage m_depthImage;
    VkDeviceMemory m_depthImageMemory;
    VkImageView m_depthImageView;
};

}