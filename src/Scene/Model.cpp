#include "Model.h"

#include <execution>
#include <iostream>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/gtc/type_ptr.hpp>

#include "GameObject.h"
#include "Descriptors/DescriptorWriter.h"
#include "Utils/Timer.h"

namespace vov {

    Model::Model(Device& deviceRef, const std::string& path, GameObject* parent): m_device{deviceRef}, m_path{path} {
        loadModel(path);
        generateMeshes(parent);
    }

    Model::Model(Device& deviceRef, const std::vector<Mesh::Builder>& builders): m_device{deviceRef}, m_builders{builders} {
        generateMeshes();
    }

    void Model::draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, bool isDepthPass) const {
        int testCounter{0};
        for (const auto& mesh: m_meshes) {
            PushConstantData push{};
            push.modelMatrix = mesh->getTransform().GetWorldMatrix();
            if (m_Owner != nullptr) {
                push.objectId = m_Owner->getId() + testCounter++;
            } else {
                push.objectId = 0;
            }
            vkCmdPushConstants(
                commandBuffer,
                pipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                sizeof(PushConstantData),
                &push
            );

            mesh->bind(commandBuffer, pipelineLayout, isDepthPass);
            mesh->draw(commandBuffer);
        }
    }

    void Model::bind(VkCommandBuffer commandBuffer, VkPipelineLayout layout) const {
        for (const auto& mesh: m_meshes) {
            mesh->bind(commandBuffer, layout);
        }
    }

    void Model::loadModel(const std::string& path) {
        Timer loadModelTimer("path");
        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_GenNormals);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cout << "ERROR::ASSIMP::" << import.GetErrorString() << std::endl;
            return;
        }
        m_directory = path.substr(0, path.find_last_of('/'));

        // PrintMaterials(scene);
        processNode(scene->mRootNode, scene);
        loadModelTimer.stop();
    }

    static glm::mat4 convertMatrix(const aiMatrix4x4& m) {
        return glm::transpose(glm::make_mat4(&m.a1));
    }

    void Model::processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform) {
        // Compute the current node's transform
        const glm::mat4 nodeTransform = convertMatrix(node->mTransformation);
        const glm::mat4 worldTransform = parentTransform * nodeTransform;

        // Process meshes for this nodes
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            Mesh::Builder builder = processMesh(mesh, scene);
            builder.transform = worldTransform; // Store the transform in the builder
            m_builders.emplace_back(std::move(builder));
        }

        // Recursively process child nodes
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene, worldTransform);
        }
    }


    Mesh::Builder Model::processMesh(aiMesh* mesh, const aiScene* scene) {
        std::vector<Mesh::Vertex> vertices;
        std::vector<uint32_t> indices;
        std::string texturePath{};

        AABB aabb{};
        if (mesh->mNumVertices > 0) {
            aabb.min = glm::vec3(mesh->mVertices[0].x, mesh->mVertices[0].y, mesh->mVertices[0].z);
            aabb.max = aabb.min;
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
            Mesh::Vertex vertex{};
            vertex.position = {
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            };
            aabb.min = glm::min(aabb.min, vertex.position);
            aabb.max = glm::max(aabb.max, vertex.position);
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
            if (mesh->HasTangentsAndBitangents()) {
                vertex.tangent = {
                    mesh->mTangents[i].x,
                    mesh->mTangents[i].y,
                    mesh->mTangents[i].z
                };

                vertex.bitTangent = {
                    mesh->mBitangents[i].x,
                    mesh->mBitangents[i].y,
                    mesh->mBitangents[i].x
                };
            }

            vertices.push_back(vertex);
        }

        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            // retrieve all indices of the face and store them in the indices vector
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }
        Mesh::Builder builder{};
        builder.vertices = std::move(vertices);
        builder.indices = std::move(indices);
        builder.modelPath = m_directory + "/";
        builder.name = mesh->mName.C_Str();
        builder.boundingBox = aabb; // Store the AABB

        Mesh::TextureInfo textureInfo{};

        if (scene->mNumMaterials > 0 && mesh->mMaterialIndex >= 0) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            aiString path;

            if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS) {
                textureInfo.albedoPath = path.C_Str();
            } else {
                textureInfo.albedoPath = "";
            }

            if (material->GetTexture(aiTextureType_NORMALS, 0, &path) == AI_SUCCESS) {
                textureInfo.normalPath = path.C_Str();
            } else {
                textureInfo.normalPath = "";
            }
            if (
                material->GetTexture(aiTextureType_SPECULAR, 0, &path) == AI_SUCCESS ||
                material->GetTexture(aiTextureType_MAYA_SPECULAR_ROUGHNESS, 0, &path) == AI_SUCCESS ||
                material->GetTexture(aiTextureType_METALNESS, 0, &path) == AI_SUCCESS
            ) {
                textureInfo.specularPath = path.C_Str();
                // std::cout << path.C_Str() << std::endl;
            } else {
                textureInfo.specularPath = "";
            }

            if (material->GetTexture(aiTextureType_HEIGHT, 0, &path) == AI_SUCCESS) {
                textureInfo.bumpPath = path.C_Str();
            } else {
                textureInfo.bumpPath = "";
            }
        }

        textureInfo.basePath = builder.modelPath;

        builder.textureInfo = textureInfo;

        return builder;
    }

    void Model::generateMeshes(GameObject* parent) {
        m_Owner = parent;
        //TODO: Fix this
        m_builders.erase(std::ranges::remove_if(m_builders,
                                                [] (const Mesh::Builder& builder) { return builder.vertices.empty(); }).begin(), m_builders.end());


        m_descriptorPool = DescriptorPool::Builder(m_device)
                .setMaxSets(static_cast<uint32_t>(m_builders.size()))
                .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(m_builders.size()))
                .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(m_builders.size()) * 4)
                .build();

        m_descriptorSetLayout = DescriptorSetLayout::Builder(m_device)
                .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
                .addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Albedo
                .addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Normal
                .addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Specular
                .addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT) // Bump
                .build();

        //TODO: fix the above to just have 1 DescriptorSetLayout and not one here and in VApp.cpp
        //Stupid stupid fix

        for (auto& builder: m_builders) {
            builder.descriptorSetLayout = m_descriptorSetLayout.get();
            builder.descriptorPool = m_descriptorPool.get();
        }

        for (const auto& builder: m_builders) {
            auto mesh = std::make_unique<Mesh>(m_device, builder);
            mesh->getTransform().SetWorldMatrix(builder.transform); // Apply transform
            if (parent) {
                mesh->getTransform().SetParent(&parent->transform, true);
            } else {
                mesh->getTransform().SetParent(nullptr, true);
            }
            m_meshes.push_back(std::move(mesh));
        }

        calculateBoundingBox();
    }

    void Model::calculateBoundingBox() {
        if (m_meshes.empty()) {
            m_boundingBox = {};
            return;
        }

        // Initialize with first mesh's transformed AABB
        m_boundingBox = TransformAABB(
            m_meshes[0]->GetBoundingBox(),
            m_meshes[0]->getTransform().GetWorldMatrix()
        );

        // Expand to include all other meshes
        for (size_t i = 1; i < m_meshes.size(); i++) {
            AABB transformed = TransformAABB(
                m_meshes[i]->GetBoundingBox(),
                m_meshes[i]->getTransform().GetWorldMatrix()
            );

            m_boundingBox.min = glm::min(m_boundingBox.min, transformed.min);
            m_boundingBox.max = glm::max(m_boundingBox.max, transformed.max);
        }
    }
}
