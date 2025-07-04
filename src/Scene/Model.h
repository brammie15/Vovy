#ifndef VMODEL_H
#define VMODEL_H

#include <vector>

#include "assimp/scene.h"
#include "Descriptors/DescriptorPool.h"
#include "Scene/Mesh.h"
#include "Utils/AABB.h"


namespace vov {

    class GameObject;

    class Model {
    public:

        struct PushConstantData {
            glm::mat4 modelMatrix{1.f};
            uint32_t objectId{0};
        };

        explicit Model(Device& deviceRef, const std::string& path, GameObject* parent = nullptr);
        Model(Device& deviceRef, const std::vector<Mesh::Builder>& builders);
        void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, bool isDepthPass = false) const;
        void bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout) const;


        [[nodiscard]] std::vector<std::unique_ptr<Mesh>>& getMeshes() { return m_meshes; }
        [[nodiscard]] const AABB& GetBoundingBox() const { return m_boundingBox; }

        std::string GetPath() { return m_path; }
        void RenderBox(const glm::vec3& color = {1.0f, 0.0f, 0.0f}) const;
    private:
        void loadModel(const std::string& path);
        void processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform = glm::mat4(1.0f));
        Mesh::Builder processMesh(aiMesh* mesh, const aiScene* scene);

        //Using nullptr so we can use the same function for with and wihtout GameObject
        void generateMeshes(GameObject* parent = nullptr);

        void calculateBoundingBox();

        GameObject* m_Owner{nullptr};

        std::string m_directory{};
        Device& m_device;
        std::vector<std::unique_ptr<Mesh>> m_meshes;

        std::string m_path{};

        AABB m_boundingBox{};

        std::vector<Mesh::Builder> m_builders;
        std::unique_ptr<DescriptorPool> m_descriptorPool;
        std::unique_ptr<DescriptorSetLayout> m_descriptorSetLayout;
    };
}

#endif //VMODEL_H
