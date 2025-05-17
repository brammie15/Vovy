#ifndef RENDERSYSTEM_H
#define RENDERSYSTEM_H

#include <memory>
#include <vector>

#include <glm/glm.hpp>
#include "core/Device.h"
#include "Rendering/Pipeline.h"
#include "Scene/GameObject.h"
#include "Utils/FrameContext.h"

namespace vov {
    struct PushConstantData {
        glm::mat4 modelMatrix{1.f};
        uint32_t objectId{0};
    };

    class GameObjectRenderSystem {
    public:
        GameObjectRenderSystem(Device& deviceRef, const std::vector<VkDescriptorSetLayout>& descriptorSetLayout);
        ~GameObjectRenderSystem();

        void renderGameObjects(const FrameContext& frameContext, const std::vector<std::unique_ptr<GameObject>>& gameObjects) const;

    private:
        void createPipelineLayout(const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);
        void createPipeline();

        Device& m_device;

        std::unique_ptr<Pipeline> m_pipeline;
        VkPipelineLayout m_pipelineLayout{};
    };
}

#endif //RENDERSYSTEM_H
