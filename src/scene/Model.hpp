#pragma once

#include <string>
#include <vector>
#include <array>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace VulkanViewer {

class VulkanDevice;

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;
    
    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::string materialName;
    uint32_t materialIndex = 0;
    
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    
    void cleanup(VulkanDevice& device);
};

struct Material {
    std::string name;
    glm::vec3 ambient = glm::vec3(0.1f);
    glm::vec3 diffuse = glm::vec3(0.8f);
    glm::vec3 specular = glm::vec3(1.0f);
    float shininess = 32.0f;
    std::string diffuseTexture;
    std::string normalTexture;
    std::string specularTexture;
    

    VkImage textureImage = VK_NULL_HANDLE;
    VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
    VkImageView textureImageView = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;
};

class Model {
public:
    Model();
    ~Model();
    
    bool loadFromFile(const std::string& filepath, VulkanDevice& device);
    bool copyFrom(const Model& other, VulkanDevice& device);
    void render(VkCommandBuffer commandBuffer);
    void cleanup(VulkanDevice& device);
    
    glm::mat4 getTransform() const { return m_transform; }
    void setTransform(const glm::mat4& transform) { m_transform = transform; }
    
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }
    
    const std::vector<Mesh>& getMeshes() const { return m_meshes; }
    const std::vector<Material>& getMaterials() const { return m_materials; }
    std::vector<Material>& getMaterials() { return m_materials; }
    
    void setMaterialTexture(size_t materialIndex, const std::string& textureType, const std::string& filepath);
    bool loadTextureToGPU(Material& material, const std::string& filepath, VulkanDevice& device);
    

    VkImageView getMaterialTextureView(size_t materialIndex) const;
    VkSampler getMaterialTextureSampler(size_t materialIndex) const;
    

    void reprocessUVCoordinates(VulkanDevice& device);
    

    bool getForceUVFlip() const { return m_forceUVFlip; }
    void setForceUVFlip(bool flip, VulkanDevice& device);
    

    void autoFixUVsForMaterial(Material& material, VulkanDevice& device);
    std::vector<glm::vec2> generateUVVariant(const Mesh& mesh, int variant);
    int detectBestUVVariant(const Mesh& mesh, const std::vector<std::vector<glm::vec2>>& uvVariants);
    

    float calculateUVCoherence(const Mesh& mesh, const std::vector<glm::vec2>& uvs);
    float calculateUVClustering(const std::vector<glm::vec2>& uvs);
    float calculateGeometricConsistency(const Mesh& mesh, const std::vector<glm::vec2>& uvs);
    float calculateTextureCoverage(const std::vector<glm::vec2>& uvs);
    float detectUVScrambling(const Mesh& mesh, const std::vector<glm::vec2>& uvs);

private:
    bool loadOBJ(const std::string& filepath, VulkanDevice& device);
    bool loadWithAssimp(const std::string& filepath, VulkanDevice& device);
    void processNode(aiNode* node, const aiScene* scene, VulkanDevice& device);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene, VulkanDevice& device);
    void subdivideMesh(Mesh& mesh, int subdivisionLevels = 2);
    void loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, Material& material);
    bool analyzeUVPattern(aiMesh* mesh);  
    void createBuffers(VulkanDevice& device);
    void createSingleMeshBuffers(Mesh& mesh, VulkanDevice& device);
    void createTexture(const std::string& texturePath, VulkanDevice& device, Material& material);
    void splitMeshByMaterials(const aiScene* scene, VulkanDevice& device); 
    
    std::string m_name;
    std::string m_directory;
    std::string m_filepath;  
    glm::mat4 m_transform = glm::mat4(1.0f);
    bool m_forceUVFlip = false;
    
    std::vector<Mesh> m_meshes;
    std::vector<Material> m_materials;
};

}