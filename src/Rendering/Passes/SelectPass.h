#ifndef SELECTPASS_H
#define SELECTPASS_H
#include "Core/Device.h"
#include "Descriptors/DescriptorPool.h"
#include "Rendering/Pipeline.h"
#include "Resources/Image.h"


//This pass will be used to select objects in the scene
//Each object has a color based on it's ID
namespace vov {
    class SelectPass {
    public:
        struct PushConstant {
            uint32_t id;
        };

        explicit SelectPass(Device& device, VkExtent2D extent);
        ~SelectPass() = default;



    private:
        Device& m_device;
        VkExtent2D m_extent;

        std::unique_ptr<DescriptorPool> m_descriptorPool;

        std::unique_ptr<Image> m_renderTarget;

        VkPipelineLayout m_pipelineLayout;
        std::unique_ptr<Pipeline> m_pipeline;

    };
}


#endif //SELECTPASS_H
