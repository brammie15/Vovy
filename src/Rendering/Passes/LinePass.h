#ifndef LINEPASS_H
#define LINEPASS_H
#include <memory>

#include "Descriptors/DescriptorPool.h"
#include "Descriptors/DescriptorSetLayout.h"
#include "Rendering/Pipeline.h"
#include "Resources/Buffer.h"
#include "Resources/Image.h"
#include "Utils/FrameContext.h"

namespace vov {
    class LinePass {
    public:
        static constexpr size_t MAX_VERTEX_COUNT = 1000000;
        struct UniformBuffer {
            glm::mat4 viewProjectionMatrix{1.f};
            glm::vec3 cameraPosition{0.f, 0.f, 0.f};
        };


        explicit LinePass(Device& deviceRef, uint32_t framesInFlight, VkExtent2D extent);
        ~LinePass();

        void Record(FrameContext context, VkCommandBuffer commandBuffer, uint32_t imageIndex, Image& depthImage);

        void Resize(VkExtent2D newSize);

        [[nodiscard]] Image& GetImage(int imageIndex) const;

    private:
        void UpdateVertexBuffer();


        Device& m_device;
        uint32_t m_framesInFlight{};

        std::unique_ptr<DescriptorPool> m_descriptorPool{};
        std::unique_ptr<DescriptorSetLayout> m_descriptorSetLayout{};
        std::vector<VkDescriptorSet> m_descriptorSets{};

        std::vector<std::unique_ptr<Buffer>> m_uniformBuffers{};

        std::vector<std::unique_ptr<Image>> m_renderTargets{};

        std::unique_ptr<Pipeline> m_pipeline{};
        VkPipelineLayout m_pipelineLayout{};

        std::unique_ptr<Buffer> m_vertexBuffer{};

        int m_vertexCount{0};
    };
}


#endif //LINEPASS_H
