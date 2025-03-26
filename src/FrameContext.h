#ifndef FRAMECONTEXT_H
#define FRAMECONTEXT_H

#include "VDevice.h"

struct FrameContext {
    int frameIndex;
    float frameTime;
    VkCommandBuffer commandBuffer;
    VkDescriptorSet globalDescriptor;
    // LveCamera &camera;
};

#endif //FRAMECONTEXT_H