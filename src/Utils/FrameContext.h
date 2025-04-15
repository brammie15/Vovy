#ifndef FRAMECONTEXT_H
#define FRAMECONTEXT_H

#include "VCamera.h"
#include "Core/VDevice.h"

struct FrameContext {
    int frameIndex;
    float frameTime;
    VkCommandBuffer commandBuffer;
    VkDescriptorSet globalDescriptor;
    VCamera &camera;
};

#endif //FRAMECONTEXT_H