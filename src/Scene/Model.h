#ifndef VMODEL_H
#define VMODEL_H

#include <vector>

#include "assimp/scene.h"
#include "Descriptors/DescriptorPool.h"
#include "Scene/Mesh.h"


namespace vov {

    class GameObject;

    class Model {
    public:
        explicit Model(Device& deviceRef, const std::string& path, GameObject* parent = nullptr);
        Model(Device& deviceRef, const std::vector<Mesh::Builder>& builders);
        void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const;
        void bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout);


        [[nodiscard]] std::vector<std::unique_ptr<Mesh>>& getMeshes() { return m_meshes; }

        void updateShadowMapDescriptorSet(VkDescriptorImageInfo descriptorSet);

    private:
        void loadModel(std::string path);
        void processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform = glm::mat4(1.0f));
        Mesh::Builder processMesh(aiMesh* mesh, const aiScene* scene);

        //Using nullptr so we can use the same function for with and wihtout GameObject
        void generateMeshes(GameObject* parent = nullptr);

        std::string m_directory{};
        Device& m_device;
        std::vector<std::unique_ptr<Mesh>> m_meshes;

        std::vector<Mesh::Builder> m_builders;
        std::unique_ptr<DescriptorPool> m_descriptorPool;
        std::unique_ptr<DescriptorSetLayout> m_descriptorSetLayout;
    };
}

#endif //VMODEL_H
