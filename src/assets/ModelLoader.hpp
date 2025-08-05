#pragma once

#include <string>
#include <memory>
#include <vector>

namespace VulkanViewer {

class Model;
class VulkanDevice;

class ModelLoader {
public:
    static std::unique_ptr<Model> loadModel(VulkanDevice& device, const std::string& filepath);
    
    static bool isSupported(const std::string& filepath);
    static std::vector<std::string> getSupportedExtensions();

private:
    static std::unique_ptr<Model> loadGLTF(VulkanDevice& device, const std::string& filepath);
    static std::unique_ptr<Model> loadFBX(VulkanDevice& device, const std::string& filepath);
    static std::unique_ptr<Model> loadOBJ(VulkanDevice& device, const std::string& filepath);
    
    static std::string getFileExtension(const std::string& filepath);
};

}