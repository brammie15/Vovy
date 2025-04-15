#include "VModel.h"

#include <execution>
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/gtc/type_ptr.hpp>

#include "Rendering/RenderSystems/GameObjectRenderSystem.h"

VModel::VModel(VDevice& deviceRef, const std::string& path, VGameObject* parent): m_device{deviceRef} {
    loadModel(path);
    generateMeshes(parent);
}

VModel::VModel(VDevice& deviceRef, const std::vector<VMesh::Builder>& builders): m_device{deviceRef}, m_builders{builders} {
    generateMeshes();
}

void VModel::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) const {
    for (const auto& mesh : m_meshes) {
        PushConstantData push{};
        push.modelMatrix = mesh->getTransform().GetWorldMatrix();

        vkCmdPushConstants(
            commandBuffer,
            pipelineLayout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(PushConstantData),
            &push
        );

        mesh->bind(commandBuffer, pipelineLayout);
        mesh->draw(commandBuffer);
    }
}

bool MeshHasOpacityMap(const aiScene* scene, const aiMesh* mesh) {
    if (!scene || !mesh || mesh->mMaterialIndex < 0) {
        return false; // Invalid scene or mesh
    }

    // Get the material associated with the mesh
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

    // Check if the material has an opacity texture
    return material->GetTextureCount(aiTextureType_OPACITY) > 0;
}

void VModel::loadModel(std::string path) {
    Assimp::Importer import;
    const aiScene *scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
        return;
    }
    m_directory = path.substr(0, path.find_last_of('/'));

    // PrintMaterials(scene);
    processNode(scene->mRootNode, scene);

}

static glm::mat4 convertMatrix(const aiMatrix4x4& m) {
    return glm::transpose(glm::make_mat4(&m.a1));
}

void VModel::processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform) {
    // Compute the current node's transform
    glm::mat4 nodeTransform = convertMatrix(node->mTransformation);
    glm::mat4 worldTransform = parentTransform * nodeTransform;

    // Process meshes for this nodes
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        VMesh::Builder builder = processMesh(mesh, scene);
        builder.transform = worldTransform; // Store the transform in the builder
        m_builders.emplace_back(std::move(builder));
    }

    // Recursively process child nodes
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, worldTransform);
    }
}



VMesh::Builder VModel::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<VMesh::Vertex> vertices;
    std::vector<uint32_t> indices;
    std::string texturePath{ "" };

    //TODO: Fuck transparency
    if (std::string(mesh->mName.C_Str()).find("decals") != std::string::npos) {
        return {};
    }

    // Retrieve material if available
    if (scene->mNumMaterials > 0 && mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        aiString path;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
            texturePath = path.C_Str();
        }
    }

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        VMesh::Vertex vertex{};
        vertex.position = {
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        };
        if (mesh->HasVertexColors(0)) {
            vertex.color = {
                mesh->mColors[0][i].r,
                mesh->mColors[0][i].g,
                mesh->mColors[0][i].b
            };
        } else {
            vertex.color = {1.0f, 1.0f, 1.0f}; // Default color
        }
        if (mesh->HasTextureCoords(0)) {
            vertex.texCoord = {
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            };
        }
        if (mesh->HasNormals()) {
            vertex.normal = {
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            };
        }

        vertices.push_back(vertex);
    }

    for(unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        // retrieve all indices of the face and store them in the indices vector
        for(unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    VMesh::Builder builder{};
    builder.vertices = std::move(vertices);
    builder.indices = std::move(indices);
    builder.texturePath = texturePath;
    builder.modelPath = m_directory + "/";
    return builder;
}

void VModel::generateMeshes(VGameObject* parent) {
    //TODO: Fix this
    m_builders.erase(std::ranges::remove_if(m_builders,
    [](const VMesh::Builder& builder) { return builder.vertices.empty(); }).begin(), m_builders.end());


    m_descriptorPool = VDescriptorPool::Builder(m_device)
        .setMaxSets(static_cast<uint32_t>(m_builders.size()))
        .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(m_builders.size()))
        .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(m_builders.size()))
        .build();

    m_descriptorSetLayout = VDescriptorSetLayout::Builder(m_device)
        .addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();

    //Stupid stupid fix

    for (auto& builder : m_builders) {
        builder.descriptorSetLayout = m_descriptorSetLayout.get();
        builder.descriptorPool = m_descriptorPool.get();
    }

    for (const auto& builder : m_builders) {
        auto mesh = std::make_unique<VMesh>(m_device, builder);
        mesh->getTransform().SetWorldMatrix(builder.transform); // Apply transform
        if (parent) {
            mesh->getTransform().SetParent(&parent->transform, true);
        }
        else {
            mesh->getTransform().SetParent(nullptr, true);
        }
        m_meshes.push_back(std::move(mesh));
    }

    // std::for_each(std::execution::par_unseq, m_builders.begin(), m_builders.end(), [this](VMesh::Builder& builder) {
    //     std::unique_ptr<VMesh> mesh = std::make_unique<VMesh>(m_device, builder);
    //     mesh->getTransform().SetWorldMatrix(builder.transform); // Apply transform
    //     m_meshes.push_back(std::move(mesh));
    // });
}
