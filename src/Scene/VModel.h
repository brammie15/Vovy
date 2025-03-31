#ifndef VMODEL_H
#define VMODEL_H

#include <vector>

#include "assimp/scene.h"
#include "Descriptors/VDescriptorPool.h"
#include "Scene/VMesh.h"

class VModel {
public:
    explicit VModel(VDevice& deviceRef, const std::string& path);
    VModel(VDevice& deviceRef, const std::vector<VMesh::Builder>& builders);
    void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const;
private:
    void loadModel(std::string path);
    void processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform = glm::mat4(1.0f));
    VMesh::Builder processMesh(aiMesh* mesh, const aiScene* scene);

    void generateMeshes();

    std::string m_directory{};
    VDevice& m_device;
    std::vector<std::unique_ptr<VMesh>> m_meshes;

    std::vector<VMesh::Builder> m_builders;
    std::unique_ptr<VDescriptorPool> m_descriptorPool;
    std::unique_ptr<VDescriptorSetLayout> m_descriptorSetLayout;
};

#endif //VMODEL_H
