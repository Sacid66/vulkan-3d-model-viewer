#include "Model.hpp"
#include "../core/VulkanDevice.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <stdexcept>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../../external/stb/stb_image.h"

namespace VulkanViewer {

VkVertexInputBindingDescription Vertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> Vertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};


    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);


    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, normal);


    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

    return attributeDescriptions;
}

void Mesh::cleanup(VulkanDevice& device) {
    if (vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device.getDevice(), vertexBuffer, nullptr);
        vertexBuffer = VK_NULL_HANDLE;
    }
    if (vertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device.getDevice(), vertexBufferMemory, nullptr);
        vertexBufferMemory = VK_NULL_HANDLE;
    }
    if (indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(device.getDevice(), indexBuffer, nullptr);
        indexBuffer = VK_NULL_HANDLE;
    }
    if (indexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(device.getDevice(), indexBufferMemory, nullptr);
        indexBufferMemory = VK_NULL_HANDLE;
    }
}

Model::Model() : m_name("Untitled"), m_transform(1.0f) {
}

Model::~Model() {
}

bool Model::loadFromFile(const std::string& filepath, VulkanDevice& device) {

    m_filepath = filepath;
    
    std::string extension = filepath.substr(filepath.find_last_of('.'));
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    

    size_t lastSlash = filepath.find_last_of("/\\");
    size_t lastDot = filepath.find_last_of('.');
    m_name = filepath.substr(lastSlash + 1, lastDot - lastSlash - 1);
    

    m_directory = filepath.substr(0, lastSlash + 1);
    

    if (extension == ".obj") {

        if (loadWithAssimp(filepath, device)) {
            return true;
        }
        return loadOBJ(filepath, device);
    } else {

        return loadWithAssimp(filepath, device);
    }
}

bool Model::copyFrom(const Model& other, VulkanDevice& device) {

    cleanup(device);
    

    m_name = other.m_name + "_copy";
    m_directory = other.m_directory;
    m_filepath = other.m_filepath;
    m_transform = other.m_transform;
    m_forceUVFlip = other.m_forceUVFlip;
    

    m_materials.reserve(other.m_materials.size());
    for (const auto& otherMaterial : other.m_materials) {
        Material material = otherMaterial;
        

        material.textureImage = VK_NULL_HANDLE;
        material.textureImageMemory = VK_NULL_HANDLE;
        material.textureImageView = VK_NULL_HANDLE;
        material.textureSampler = VK_NULL_HANDLE;
        

        if (otherMaterial.textureImage != VK_NULL_HANDLE && !material.diffuseTexture.empty()) {
            if (!loadTextureToGPU(material, material.diffuseTexture, device)) {
                std::cerr << "Failed to copy texture: " << material.diffuseTexture << std::endl;
            }
        }
        
        m_materials.push_back(material);
    }
    

    m_meshes.reserve(other.m_meshes.size());
    for (const auto& otherMesh : other.m_meshes) {
        Mesh mesh;
        mesh.vertices = otherMesh.vertices;
        mesh.indices = otherMesh.indices;
        mesh.materialName = otherMesh.materialName;
        mesh.materialIndex = otherMesh.materialIndex;
        

        mesh.vertexBuffer = VK_NULL_HANDLE;
        mesh.vertexBufferMemory = VK_NULL_HANDLE;
        mesh.indexBuffer = VK_NULL_HANDLE;
        mesh.indexBufferMemory = VK_NULL_HANDLE;
        

        createSingleMeshBuffers(mesh, device);
        
        m_meshes.push_back(mesh);
    }
    
    return true;
}

void Model::render(VkCommandBuffer commandBuffer) {
    for (const auto& mesh : m_meshes) {
        VkBuffer vertexBuffers[] = {mesh.vertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
    }
}

void Model::cleanup(VulkanDevice& device) {
    for (auto& mesh : m_meshes) {
        if (mesh.vertexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device.getDevice(), mesh.vertexBuffer, nullptr);
            mesh.vertexBuffer = VK_NULL_HANDLE;
        }
        if (mesh.vertexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device.getDevice(), mesh.vertexBufferMemory, nullptr);
            mesh.vertexBufferMemory = VK_NULL_HANDLE;
        }
        if (mesh.indexBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device.getDevice(), mesh.indexBuffer, nullptr);
            mesh.indexBuffer = VK_NULL_HANDLE;
        }
        if (mesh.indexBufferMemory != VK_NULL_HANDLE) {
            vkFreeMemory(device.getDevice(), mesh.indexBufferMemory, nullptr);
            mesh.indexBufferMemory = VK_NULL_HANDLE;
        }
    }
    m_meshes.clear();
    m_materials.clear();
}

bool Model::loadWithAssimp(const std::string& filepath, VulkanDevice& device) {
    std::cout << "Loading model with transform pre-processing and UV analysis..." << std::endl;
    
    Assimp::Importer importer;
    

    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
    

    importer.SetPropertyInteger(AI_CONFIG_PP_SLM_TRIANGLE_LIMIT, 10000000);  
    importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 10000000);      
    importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 180.0f); 
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, true);    
    

    const aiScene* scene = importer.ReadFile(filepath, 
        aiProcess_Triangulate |          
        aiProcess_FlipUVs                

    );
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Assimp Error: " << importer.GetErrorString() << std::endl;
        return false;
    }
    
   
    m_materials.reserve(scene->mNumMaterials);
    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* mat = scene->mMaterials[i];
        Material material;
        
        aiString name;
        mat->Get(AI_MATKEY_NAME, name);
        material.name = name.C_Str();
        
        aiColor3D color;
        if (mat->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) {
            material.ambient = glm::vec3(color.r, color.g, color.b);
        }
        if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            material.diffuse = glm::vec3(color.r, color.g, color.b);
        }
        if (mat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
            material.specular = glm::vec3(color.r, color.g, color.b);
        }
        
        float shininess;
        if (mat->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
            material.shininess = shininess;
        }
        
     
        loadMaterialTextures(mat, aiTextureType_DIFFUSE, "diffuse", material);
        loadMaterialTextures(mat, aiTextureType_NORMALS, "normal", material);
        loadMaterialTextures(mat, aiTextureType_SPECULAR, "specular", material);
        
        m_materials.push_back(material);
    }
    
    
    processNode(scene->mRootNode, scene, device);
    
    
    int textureCount = 0;
    for (size_t i = 0; i < m_materials.size(); i++) {
        auto& material = m_materials[i];
        if (!material.diffuseTexture.empty()) {
            if (loadTextureToGPU(material, material.diffuseTexture, device)) {
                textureCount++;
            } else {
                std::cerr << "ERROR: Failed to load texture for material " << i << std::endl;
            }
        }
    }
    
    if (textureCount > 0) {
        std::cout << "Loaded " << textureCount << " textures to GPU" << std::endl;
    }
    
    
    m_transform = glm::mat4(1.0f);
    
    std::cout << "Successfully loaded model: " << filepath << std::endl;
    std::cout << "Meshes: " << m_meshes.size() << ", Materials: " << m_materials.size() << std::endl;
    

    bool hasTextureIssues = false;
    for (size_t i = 0; i < m_materials.size(); i++) {
        if (m_materials[i].diffuseTexture.empty()) {
            hasTextureIssues = true;
            break;
        }
    }
    
    if (hasTextureIssues) {
        std::cout << "WARNING: Some materials have no textures - model may appear black/gray" << std::endl;
        std::cout << "You can manually load textures from the Properties panel" << std::endl;
    }
    
    return true;
}

void Model::processNode(aiNode* node, const aiScene* scene, VulkanDevice& device) {

    aiMatrix4x4 nodeTransform = node->mTransformation;
    
    
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        Mesh processedMesh = processMesh(mesh, scene, device);
        
      
        if (!nodeTransform.IsIdentity()) {
            for (auto& vertex : processedMesh.vertices) {
                aiVector3D pos(vertex.pos.x, vertex.pos.y, vertex.pos.z);
                pos = nodeTransform * pos;
                vertex.pos = glm::vec3(pos.x, pos.y, pos.z);
                
               
                aiVector3D normal(vertex.normal.x, vertex.normal.y, vertex.normal.z);
                aiMatrix4x4 normalMatrix = nodeTransform;
                normalMatrix.Inverse().Transpose();
                normal = normalMatrix * normal;
                vertex.normal = glm::normalize(glm::vec3(normal.x, normal.y, normal.z));
            }
            
            
            processedMesh.cleanup(device);
            createSingleMeshBuffers(processedMesh, device);
        }
        
        m_meshes.push_back(processedMesh);
    }
    

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, device);
    }
}

Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene, VulkanDevice& device) {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    std::cout << "  Processing mesh with " << mesh->mNumVertices << " vertices, " << mesh->mNumFaces << " faces" << std::endl;
    
    
    
   
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        
       
        vertex.pos = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        
       
        if (mesh->HasNormals()) {
            vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        } else {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        
       
        if (mesh->mTextureCoords[0]) {
            const aiVector3D& uvCoord = mesh->mTextureCoords[0][i];
            vertex.texCoord = glm::vec2(uvCoord.x, uvCoord.y);
        } else {
            vertex.texCoord = glm::vec2(0.0f, 0.0f);
        }
        
        vertices.push_back(vertex);
    }
    
   
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }
    
    
    Mesh resultMesh;
    resultMesh.vertices = vertices;
    resultMesh.indices = indices;
    
    
    if (!m_materials.empty() && mesh->mMaterialIndex < m_materials.size()) {
        resultMesh.materialName = m_materials[mesh->mMaterialIndex].name;
        resultMesh.materialIndex = mesh->mMaterialIndex;
        std::cout << "    Material: " << mesh->mMaterialIndex << " (" << resultMesh.materialName << ")" << std::endl;
    } else {
        
        if (m_materials.empty()) {
            Material defaultMaterial;
            defaultMaterial.name = "default";
            defaultMaterial.diffuse = glm::vec3(0.7f, 0.7f, 0.7f);
            m_materials.push_back(defaultMaterial);
            std::cout << "    Created default material" << std::endl;
        }
        
        resultMesh.materialName = m_materials[0].name;
        resultMesh.materialIndex = 0;
        
        if (mesh->mMaterialIndex >= m_materials.size()) {
            std::cout << "    WARNING: Invalid material index " << mesh->mMaterialIndex 
                      << ", using material 0" << std::endl;
        }
    }
    
 
    
    
    createSingleMeshBuffers(resultMesh, device);
    
    return resultMesh;
}

void Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, const std::string& typeName, Material& material) {
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        std::string textureFilename = str.C_Str();
        
       
        

        std::vector<std::string> possiblePaths = {
            m_directory + textureFilename,                          
            m_directory + "../textures/" + textureFilename,          
            m_directory + "textures/" + textureFilename,            
            m_directory + "../" + textureFilename,                   
        };
        

        size_t dotPos = textureFilename.find_last_of('.');
        if (dotPos == std::string::npos) {

            std::vector<std::string> extensions = {".png", ".jpg", ".jpeg", ".tga", ".bmp"};
            for (const auto& basePath : possiblePaths) {
                for (const auto& ext : extensions) {
                    possiblePaths.push_back(basePath + ext);
                }
            }
        }
        
        std::string finalTexturePath;
        bool found = false;
        

        for (const auto& path : possiblePaths) {
            std::ifstream file(path);
            if (file.good()) {
                finalTexturePath = path;
                found = true;
               
                break;
            }
        }
        
        if (!found) {

            return;
        }
        
        if (typeName == "diffuse") {
            material.diffuseTexture = finalTexturePath;
        } else if (typeName == "normal") {
            material.normalTexture = finalTexturePath;
        } else if (typeName == "specular") {
            material.specularTexture = finalTexturePath;
        }
    }
}

bool Model::loadOBJ(const std::string& filepath, VulkanDevice& device) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filepath << std::endl;
        return false;
    }


    size_t lastSlash = filepath.find_last_of("/\\");
    size_t lastDot = filepath.find_last_of('.');
    m_name = filepath.substr(lastSlash + 1, lastDot - lastSlash - 1);
    

    m_directory = filepath.substr(0, lastSlash + 1);

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<std::string, uint32_t> uniqueVertices;
    
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") {

            glm::vec3 pos;
            iss >> pos.x >> pos.y >> pos.z;
            positions.push_back(pos);
        } else if (prefix == "vn") {
           
            glm::vec3 normal;
            iss >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        } else if (prefix == "vt") {

            glm::vec2 texCoord;
            iss >> texCoord.x >> texCoord.y;
            texCoords.push_back(texCoord);
        } else if (prefix == "f") {

            std::string vertexStr;
            std::vector<std::string> faceVertices;
            
            while (iss >> vertexStr) {
                faceVertices.push_back(vertexStr);
            }
            

            for (size_t i = 1; i < faceVertices.size() - 1; i++) {
                for (size_t j = 0; j < 3; j++) {
                    size_t idx = (j == 0) ? 0 : i + j - 1;
                    std::string vertex = faceVertices[idx];
                    
                    if (uniqueVertices.count(vertex) == 0) {
                        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                        
                        Vertex v{};
                        
                        
                        std::istringstream viss(vertex);
                        std::string posStr, texStr, normalStr;
                        
                        std::getline(viss, posStr, '/');
                        std::getline(viss, texStr, '/');
                        std::getline(viss, normalStr, '/');
                        
                      
                        int posIdx = std::stoi(posStr) - 1;
                        if (posIdx >= 0 && posIdx < positions.size()) {
                            v.pos = positions[posIdx];
                        }
                        
                    
                        if (!texStr.empty()) {
                            int texIdx = std::stoi(texStr) - 1;
                            if (texIdx >= 0 && texIdx < texCoords.size()) {
                                v.texCoord = texCoords[texIdx];
                            }
                        }
                        
                     
                        if (!normalStr.empty()) {
                            int normalIdx = std::stoi(normalStr) - 1;
                            if (normalIdx >= 0 && normalIdx < normals.size()) {
                                v.normal = normals[normalIdx];
                            }
                        } else {
                           
                            v.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                        }
                        
                        vertices.push_back(v);
                    }
                    
                    indices.push_back(uniqueVertices[vertex]);
                }
            }
        }
    }
    
    file.close();
    
    if (vertices.empty()) {
        std::cerr << "No vertices found in OBJ file: " << filepath << std::endl;
        return false;
    }
    

    Mesh mesh;
    mesh.vertices = vertices;
    mesh.indices = indices;
    mesh.materialName = "default";
    
    m_meshes.push_back(mesh);
    

    Material defaultMaterial;
    defaultMaterial.name = "default";
    defaultMaterial.ambient = glm::vec3(0.2f);
    defaultMaterial.diffuse = glm::vec3(0.8f);
    defaultMaterial.specular = glm::vec3(1.0f);
    defaultMaterial.shininess = 32.0f;
    m_materials.push_back(defaultMaterial);
    

    createBuffers(device);
    

    m_transform = glm::mat4(1.0f);  
    
    std::cout << "Successfully loaded OBJ file: " << filepath << std::endl;
    std::cout << "Vertices: " << vertices.size() << ", Indices: " << indices.size() << std::endl;
    
    return true;
}

void Model::createSingleMeshBuffers(Mesh& mesh, VulkanDevice& device) {

    VkDeviceSize vertexBufferSize = sizeof(mesh.vertices[0]) * mesh.vertices.size();
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    device.createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      stagingBuffer, stagingBufferMemory);
    
    void* data;
    vkMapMemory(device.getDevice(), stagingBufferMemory, 0, vertexBufferSize, 0, &data);
    memcpy(data, mesh.vertices.data(), static_cast<size_t>(vertexBufferSize));
    vkUnmapMemory(device.getDevice(), stagingBufferMemory);
    
    device.createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh.vertexBuffer, mesh.vertexBufferMemory);
    
    device.copyBuffer(stagingBuffer, mesh.vertexBuffer, vertexBufferSize);
    
    vkDestroyBuffer(device.getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(device.getDevice(), stagingBufferMemory, nullptr);
    

    VkDeviceSize indexBufferSize = sizeof(mesh.indices[0]) * mesh.indices.size();
    
    device.createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      stagingBuffer, stagingBufferMemory);
    
    vkMapMemory(device.getDevice(), stagingBufferMemory, 0, indexBufferSize, 0, &data);
    memcpy(data, mesh.indices.data(), static_cast<size_t>(indexBufferSize));
    vkUnmapMemory(device.getDevice(), stagingBufferMemory);
    
    device.createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh.indexBuffer, mesh.indexBufferMemory);
    
    device.copyBuffer(stagingBuffer, mesh.indexBuffer, indexBufferSize);
    
    vkDestroyBuffer(device.getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(device.getDevice(), stagingBufferMemory, nullptr);
}

void Model::createBuffers(VulkanDevice& device) {
    for (auto& mesh : m_meshes) {

        VkDeviceSize vertexBufferSize = sizeof(mesh.vertices[0]) * mesh.vertices.size();
        
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        device.createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          stagingBuffer, stagingBufferMemory);
        
        void* data;
        vkMapMemory(device.getDevice(), stagingBufferMemory, 0, vertexBufferSize, 0, &data);
        memcpy(data, mesh.vertices.data(), static_cast<size_t>(vertexBufferSize));
        vkUnmapMemory(device.getDevice(), stagingBufferMemory);
        
        device.createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh.vertexBuffer, mesh.vertexBufferMemory);
        
        device.copyBuffer(stagingBuffer, mesh.vertexBuffer, vertexBufferSize);
        
        vkDestroyBuffer(device.getDevice(), stagingBuffer, nullptr);
        vkFreeMemory(device.getDevice(), stagingBufferMemory, nullptr);
        

        VkDeviceSize indexBufferSize = sizeof(mesh.indices[0]) * mesh.indices.size();
        
        device.createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                          stagingBuffer, stagingBufferMemory);
        
        vkMapMemory(device.getDevice(), stagingBufferMemory, 0, indexBufferSize, 0, &data);
        memcpy(data, mesh.indices.data(), static_cast<size_t>(indexBufferSize));
        vkUnmapMemory(device.getDevice(), stagingBufferMemory);
        
        device.createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mesh.indexBuffer, mesh.indexBufferMemory);
        
        device.copyBuffer(stagingBuffer, mesh.indexBuffer, indexBufferSize);
        
        vkDestroyBuffer(device.getDevice(), stagingBuffer, nullptr);
        vkFreeMemory(device.getDevice(), stagingBufferMemory, nullptr);
    }
}

void Model::subdivideMesh(Mesh& mesh, int subdivisionLevels) {
    for (int level = 0; level < subdivisionLevels; ++level) {
        std::vector<Vertex> newVertices = mesh.vertices;
        std::vector<uint32_t> newIndices;
        

        for (size_t i = 0; i < mesh.indices.size(); i += 3) {
            uint32_t v0 = mesh.indices[i];
            uint32_t v1 = mesh.indices[i + 1];
            uint32_t v2 = mesh.indices[i + 2];
            

            Vertex midV01, midV12, midV20;
            
            midV01.pos = (mesh.vertices[v0].pos + mesh.vertices[v1].pos) * 0.5f;
            midV01.normal = glm::normalize((mesh.vertices[v0].normal + mesh.vertices[v1].normal) * 0.5f);
            midV01.texCoord = (mesh.vertices[v0].texCoord + mesh.vertices[v1].texCoord) * 0.5f;
            
            midV12.pos = (mesh.vertices[v1].pos + mesh.vertices[v2].pos) * 0.5f;
            midV12.normal = glm::normalize((mesh.vertices[v1].normal + mesh.vertices[v2].normal) * 0.5f);
            midV12.texCoord = (mesh.vertices[v1].texCoord + mesh.vertices[v2].texCoord) * 0.5f;
            
            midV20.pos = (mesh.vertices[v2].pos + mesh.vertices[v0].pos) * 0.5f;
            midV20.normal = glm::normalize((mesh.vertices[v2].normal + mesh.vertices[v0].normal) * 0.5f);
            midV20.texCoord = (mesh.vertices[v2].texCoord + mesh.vertices[v0].texCoord) * 0.5f;
            

            uint32_t midIdx01 = newVertices.size();
            newVertices.push_back(midV01);
            uint32_t midIdx12 = newVertices.size();
            newVertices.push_back(midV12);
            uint32_t midIdx20 = newVertices.size();
            newVertices.push_back(midV20);
            

            newIndices.push_back(v0);
            newIndices.push_back(midIdx01);
            newIndices.push_back(midIdx20);
            

            newIndices.push_back(midIdx01);
            newIndices.push_back(v1);
            newIndices.push_back(midIdx12);
            

            newIndices.push_back(midIdx20);
            newIndices.push_back(midIdx12);
            newIndices.push_back(v2);
            

            newIndices.push_back(midIdx01);
            newIndices.push_back(midIdx12);
            newIndices.push_back(midIdx20);
        }
        
        mesh.vertices = newVertices;
        mesh.indices = newIndices;
    }
}

void Model::setMaterialTexture(size_t materialIndex, const std::string& textureType, const std::string& filepath) {
    if (materialIndex >= m_materials.size()) {
        std::cerr << "Invalid material index: " << materialIndex << std::endl;
        return;
    }
    
    Material& material = m_materials[materialIndex];
    
    if (textureType == "diffuse") {
        material.diffuseTexture = filepath;
    } else if (textureType == "normal") {
        material.normalTexture = filepath;
    } else if (textureType == "specular") {
        material.specularTexture = filepath;
    } else {
        std::cerr << "Unknown texture type: " << textureType << std::endl;
        return;
    }
    
    std::cout << "Set " << textureType << " texture for material " << materialIndex << ": " << filepath << std::endl;
    std::cout << "  WARNING: Texture set but not loaded to GPU! Call loadTextureToGPU() after this!" << std::endl;
}

bool Model::loadTextureToGPU(Material& material, const std::string& filepath, VulkanDevice& device) {
    std::cout << "Loading texture with AUTO-UV-FIX: " << filepath << std::endl;
    

    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    
    if (!pixels) {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        return false;
    }
    
    
    std::cout << "=== AUTOMATIC UV FIX: Trying all variants to find the best one ===" << std::endl;
    autoFixUVsForMaterial(material, device);
    std::cout << "=== UV FIX COMPLETE! Texture should now map correctly ===" << std::endl;
    
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    device.createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       stagingBuffer, stagingBufferMemory);
    

    void* data;
    vkMapMemory(device.getDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device.getDevice(), stagingBufferMemory);
    
    stbi_image_free(pixels);
    

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(texWidth);
    imageInfo.extent.height = static_cast<uint32_t>(texHeight);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateImage(device.getDevice(), &imageInfo, nullptr, &material.textureImage) != VK_SUCCESS) {
        std::cerr << "Failed to create texture image!" << std::endl;
        vkDestroyBuffer(device.getDevice(), stagingBuffer, nullptr);
        vkFreeMemory(device.getDevice(), stagingBufferMemory, nullptr);
        return false;
    }
    

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device.getDevice(), material.textureImage, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    if (vkAllocateMemory(device.getDevice(), &allocInfo, nullptr, &material.textureImageMemory) != VK_SUCCESS) {
        std::cerr << "Failed to allocate texture image memory!" << std::endl;
        vkDestroyImage(device.getDevice(), material.textureImage, nullptr);
        vkDestroyBuffer(device.getDevice(), stagingBuffer, nullptr);
        vkFreeMemory(device.getDevice(), stagingBufferMemory, nullptr);
        return false;
    }
    
    vkBindImageMemory(device.getDevice(), material.textureImage, material.textureImageMemory, 0);
    

    VkCommandBuffer commandBuffer = device.beginSingleTimeCommands();
    
   
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = material.textureImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                        0, 0, nullptr, 0, nullptr, 1, &barrier);
    

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1};
    
    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, material.textureImage, 
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    device.endSingleTimeCommands(commandBuffer);
    

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = material.textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    if (vkCreateImageView(device.getDevice(), &viewInfo, nullptr, &material.textureImageView) != VK_SUCCESS) {
        std::cerr << "Failed to create texture image view!" << std::endl;
        return false;
    }
    

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    
    if (vkCreateSampler(device.getDevice(), &samplerInfo, nullptr, &material.textureSampler) != VK_SUCCESS) {
        std::cerr << "Failed to create texture sampler!" << std::endl;
        return false;
    }
    

    vkDestroyBuffer(device.getDevice(), stagingBuffer, nullptr);
    vkFreeMemory(device.getDevice(), stagingBufferMemory, nullptr);
    
    
    return true;
}

VkImageView Model::getMaterialTextureView(size_t materialIndex) const {
    if (materialIndex >= m_materials.size()) {
        return VK_NULL_HANDLE;
    }
    return m_materials[materialIndex].textureImageView;
}

VkSampler Model::getMaterialTextureSampler(size_t materialIndex) const {
    if (materialIndex >= m_materials.size()) {
        return VK_NULL_HANDLE;
    }
    return m_materials[materialIndex].textureSampler;
}

bool Model::analyzeUVPattern(aiMesh* mesh) {
    if (!mesh->mTextureCoords[0] || mesh->mNumVertices < 3) {
        return false; 
    }
    

    std::vector<glm::vec2> sampleUVs;
    int sampleCount = std::min(static_cast<int>(mesh->mNumVertices), 100); 
    int step = std::max(1, static_cast<int>(mesh->mNumVertices) / sampleCount);
    
    for (int i = 0; i < mesh->mNumVertices; i += step) {
        if (sampleUVs.size() >= sampleCount) break;
        const aiVector3D& uvCoord = mesh->mTextureCoords[0][i];
        sampleUVs.push_back(glm::vec2(uvCoord.x, uvCoord.y));
    }
    

    float minV = FLT_MAX, maxV = -FLT_MAX, avgV = 0.0f;
    int zeroVCount = 0, oneVCount = 0, midVCount = 0;
    
    for (const auto& uv : sampleUVs) {
        float v = uv.y;
        minV = std::min(minV, v);
        maxV = std::max(maxV, v);
        avgV += v;
        
        if (v < 0.1f) zeroVCount++;        
        else if (v > 0.9f) oneVCount++;    
        else if (v > 0.3f && v < 0.7f) midVCount++; 
    }
    avgV /= sampleUVs.size();
    
    
    bool flipHeuristic1 = (avgV > 0.45f); 
    bool flipHeuristic2 = (oneVCount >= zeroVCount);  
    bool flipHeuristic3 = (minV > 0.05f && maxV > 0.5f);  
    bool flipHeuristic4 = (midVCount < sampleUVs.size() / 2);  
    bool flipHeuristic5 = (maxV - minV > 0.3f && avgV > 0.45f);  
    bool flipHeuristic6 = (minV > 0.2f);  
    
    
    int flipVotes = 0;
    if (flipHeuristic1) flipVotes++;
    if (flipHeuristic2) flipVotes++;
    if (flipHeuristic3) flipVotes++;
    if (flipHeuristic4) flipVotes++;
    if (flipHeuristic5) flipVotes++;
    if (flipHeuristic6) flipVotes++;
    
    bool needsFlip = (flipVotes >= 1);  
    

    std::cout << "    UV Analysis: MinV=" << minV << ", MaxV=" << maxV << ", AvgV=" << avgV 
              << " | Zero=" << zeroVCount << ", One=" << oneVCount << ", Mid=" << midVCount
              << " | Votes=" << flipVotes << "/6 -> " << (needsFlip ? "FLIP" : "NO FLIP") << std::endl;
    
    return needsFlip;
}

void Model::reprocessUVCoordinates(VulkanDevice& device) {
    std::cout << "Reprocessing UV coordinates with flip=" << m_forceUVFlip << " for model: " << m_name << std::endl;
    

    glm::mat4 savedTransform = m_transform;
    std::string savedName = m_name;
    

    cleanup(device);
    

    m_meshes.clear();
    

    std::string extension = m_filepath.substr(m_filepath.find_last_of('.'));
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    
    if (extension == ".obj") {
        loadWithAssimp(m_filepath, device);
    } else {
        loadWithAssimp(m_filepath, device);
    }
    

    m_transform = savedTransform;
    m_name = savedName;
    
    std::cout << "UV reprocessing complete for model: " << m_name << std::endl;
}

void Model::setForceUVFlip(bool flip, VulkanDevice& device) {
    if (m_forceUVFlip != flip) {
        m_forceUVFlip = flip;
        reprocessUVCoordinates(device);
    }
}

void Model::autoFixUVsForMaterial(Material& material, VulkanDevice& device) {
    std::cout << "SIMPLE UV FIX: Applying basic UV mapping for material: " << material.name << std::endl;

    for (size_t meshIndex = 0; meshIndex < m_meshes.size(); meshIndex++) {
        auto& mesh = m_meshes[meshIndex];
        if (mesh.materialIndex < m_materials.size() && &m_materials[mesh.materialIndex] == &material) {
            std::cout << "  Processing mesh " << meshIndex << " with " << mesh.vertices.size() << " vertices" << std::endl;
            
            // Simple UV normalization - ensure UVs are in 0-1 range
            for (auto& vertex : mesh.vertices) {
                // Clamp UV coordinates to 0-1 range
                vertex.texCoord.x = std::max(0.0f, std::min(1.0f, vertex.texCoord.x));
                vertex.texCoord.y = std::max(0.0f, std::min(1.0f, vertex.texCoord.y));
                
                // Optional: flip Y coordinate if needed (common for different texture coordinate systems)
                // vertex.texCoord.y = 1.0f - vertex.texCoord.y;
            }
            
            // Update GPU buffers
            mesh.cleanup(device);
            createSingleMeshBuffers(mesh, device);
            
            std::cout << "  UV coordinates normalized for mesh " << meshIndex << std::endl;
        }
    }
    
    std::cout << "Simple UV fix complete for material: " << material.name << std::endl;
}

std::vector<glm::vec2> Model::generateUVVariant(const Mesh& mesh, int variant) {
    std::vector<glm::vec2> uvs;
    uvs.reserve(mesh.vertices.size());
    
    std::cout << "      Generating UV variant " << variant << " from " << mesh.vertices.size() << " vertices" << std::endl;
    
    for (const auto& vertex : mesh.vertices) {
        glm::vec2 uv = vertex.texCoord;
        glm::vec2 originalUV = uv;  
        
        switch (variant) {
            case 0: 
                break;
            case 1: 
                uv.y = 1.0f - uv.y;
                break;
            case 2: 
                uv.x = 1.0f - uv.x;
                break;
            case 3: 
                uv.x = 1.0f - uv.x;
                uv.y = 1.0f - uv.y;
                break;
        }
        

        if (uvs.empty() && variant > 0) {
            std::cout << "        First UV: (" << originalUV.x << "," << originalUV.y 
                      << ") -> (" << uv.x << "," << uv.y << ")" << std::endl;
        }
        
        uvs.push_back(uv);
    }
    
    return uvs;
}

int Model::detectBestUVVariant(const Mesh& mesh, const std::vector<std::vector<glm::vec2>>& uvVariants) {

    
    float bestScore = -1.0f;
    int bestVariant = 0;  
    
    std::cout << "    ADVANCED ANALYSIS: Testing " << uvVariants.size() << " UV variants..." << std::endl;
    
    for (int v = 0; v < uvVariants.size(); v++) {
        const auto& uvs = uvVariants[v];
        

        float coherenceScore = calculateUVCoherence(mesh, uvs);
        

        float clusterScore = calculateUVClustering(uvs);
        

        float geometryScore = calculateGeometricConsistency(mesh, uvs);
        

        float coverageScore = calculateTextureCoverage(uvs);
        

        float scramblingPenalty = detectUVScrambling(mesh, uvs);
        

        float score = coherenceScore * 0.4f + clusterScore * 0.2f + 
                      geometryScore * 0.2f + coverageScore * 0.1f - scramblingPenalty * 0.1f;
        
        std::cout << "    Variant " << v << ": coherence=" << coherenceScore 
                  << " cluster=" << clusterScore << " geometry=" << geometryScore 
                  << " coverage=" << coverageScore << " scrambling=" << scramblingPenalty
                  << " TOTAL=" << score << std::endl;
        
        if (score > bestScore) {
            bestScore = score;
            bestVariant = v;
        }
    }
    
    std::cout << "    ADVANCED RESULT: Best variant = " << bestVariant << " (score: " << bestScore << ")" << std::endl;
    return bestVariant;
}

float Model::calculateUVCoherence(const Mesh& mesh, const std::vector<glm::vec2>& uvs) {
    
    
    float totalCoherence = 0.0f;
    int coherentPairs = 0;
    

    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        if (i + 2 < mesh.indices.size()) {
            uint32_t idx1 = mesh.indices[i];
            uint32_t idx2 = mesh.indices[i + 1];
            uint32_t idx3 = mesh.indices[i + 2];
            
            if (idx1 < uvs.size() && idx2 < uvs.size() && idx3 < uvs.size()) {
                glm::vec2 uv1 = uvs[idx1];
                glm::vec2 uv2 = uvs[idx2];
                glm::vec2 uv3 = uvs[idx3];
                

                float dist12 = glm::length(uv2 - uv1);
                float dist23 = glm::length(uv3 - uv2);
                float dist31 = glm::length(uv1 - uv3);
                

                float triangleCoherence = 1.0f / (1.0f + dist12 + dist23 + dist31);
                totalCoherence += triangleCoherence;
                coherentPairs++;
            }
        }
    }
    
    return coherentPairs > 0 ? totalCoherence / coherentPairs : 0.0f;
}

float Model::calculateUVClustering(const std::vector<glm::vec2>& uvs) {

    
    const int gridSize = 8;
    int grid[gridSize][gridSize] = {0};
    int totalUVs = 0;
    

    for (const auto& uv : uvs) {
        if (uv.x >= 0.0f && uv.x <= 1.0f && uv.y >= 0.0f && uv.y <= 1.0f) {
            int gx = std::min(gridSize-1, (int)(uv.x * gridSize));
            int gy = std::min(gridSize-1, (int)(uv.y * gridSize));
            grid[gx][gy]++;
            totalUVs++;
        }
    }
    
    if (totalUVs == 0) return 0.0f;
    
   
    float clusterScore = 0.0f;
    int activeCells = 0;
    
    for (int x = 0; x < gridSize; x++) {
        for (int y = 0; y < gridSize; y++) {
            if (grid[x][y] > 0) {
                activeCells++;
                
                float density = (float)grid[x][y] / totalUVs;
                clusterScore += density * density; 
            }
        }
    }
    

    float scatterPenalty = (float)activeCells / (gridSize * gridSize);
    return clusterScore - scatterPenalty * 0.5f;
}

float Model::calculateGeometricConsistency(const Mesh& mesh, const std::vector<glm::vec2>& uvs) {

    
    float totalConsistency = 0.0f;
    int consistentPairs = 0;

    for (size_t i = 0; i < mesh.indices.size() && i < 300; i += 3) {
        if (i + 2 < mesh.indices.size()) {
            uint32_t idx1 = mesh.indices[i];
            uint32_t idx2 = mesh.indices[i + 1];
            
            if (idx1 < mesh.vertices.size() && idx2 < mesh.vertices.size() && 
                idx1 < uvs.size() && idx2 < uvs.size()) {
                

                glm::vec3 pos1 = mesh.vertices[idx1].pos;
                glm::vec3 pos2 = mesh.vertices[idx2].pos;
                float geoDist = glm::length(pos2 - pos1);
                

                glm::vec2 uv1 = uvs[idx1];
                glm::vec2 uv2 = uvs[idx2];
                float uvDist = glm::length(uv2 - uv1);
                
                
                if (geoDist > 0.001f && uvDist > 0.001f) {
                    float ratio = std::min(geoDist / uvDist, uvDist / geoDist);
                    totalConsistency += ratio;
                    consistentPairs++;
                }
            }
        }
    }
    
    return consistentPairs > 0 ? totalConsistency / consistentPairs : 0.0f;
}

float Model::calculateTextureCoverage(const std::vector<glm::vec2>& uvs) {

    
    const int gridSize = 16;
    bool grid[gridSize][gridSize] = {false};
    int validUVs = 0;
    
    for (const auto& uv : uvs) {
        if (uv.x >= 0.0f && uv.x <= 1.0f && uv.y >= 0.0f && uv.y <= 1.0f) {
            int gx = std::min(gridSize-1, (int)(uv.x * gridSize));
            int gy = std::min(gridSize-1, (int)(uv.y * gridSize));
            grid[gx][gy] = true;
            validUVs++;
        }
    }
    

    int usedCells = 0;
    for (int x = 0; x < gridSize; x++) {
        for (int y = 0; y < gridSize; y++) {
            if (grid[x][y]) usedCells++;
        }
    }
    
    float coverage = (float)usedCells / (gridSize * gridSize);
    float validRatio = uvs.empty() ? 0.0f : (float)validUVs / uvs.size();
    
    return coverage * validRatio;
}

float Model::detectUVScrambling(const Mesh& mesh, const std::vector<glm::vec2>& uvs) {

    
    float totalVariance = 0.0f;
    int varianceSamples = 0;
    
    
    for (size_t i = 0; i < mesh.indices.size() && i < 600; i += 3) { 
        if (i + 2 < mesh.indices.size()) {
            uint32_t idx1 = mesh.indices[i];
            uint32_t idx2 = mesh.indices[i + 1];
            uint32_t idx3 = mesh.indices[i + 2];
            
            if (idx1 < uvs.size() && idx2 < uvs.size() && idx3 < uvs.size()) {
                glm::vec2 uv1 = uvs[idx1];
                glm::vec2 uv2 = uvs[idx2];
                glm::vec2 uv3 = uvs[idx3];
                

                glm::vec2 center = (uv1 + uv2 + uv3) / 3.0f;
                float variance = glm::length(uv1 - center) + 
                                glm::length(uv2 - center) + 
                                glm::length(uv3 - center);
                
                totalVariance += variance;
                varianceSamples++;
            }
        }
    }
    
    float avgVariance = varianceSamples > 0 ? totalVariance / varianceSamples : 0.0f;
    

    return avgVariance > 0.3f ? avgVariance : 0.0f;
}

}