#ifndef FRAMECONTEXT_H
#define FRAMECONTEXT_H

#include "Camera.h"
#include "Core/Device.h"

namespace vov {
    struct FrameContext {
        int frameIndex;
        float frameTime;
        VkCommandBuffer commandBuffer;
        VkDescriptorSet globalDescriptor;
        Camera& camera;
    };
}

#endif //FRAMECONTEXT_H
