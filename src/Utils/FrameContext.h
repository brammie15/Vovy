#ifndef FRAMECONTEXT_H
#define FRAMECONTEXT_H

#include "Camera.h"
#include "Core/Device.h"

namespace vov {
    class Scene;

    enum class DebugView {
        NONE,
        ALBEDO,
        NORMAL,
        SPECULAR,
        ROUGHNESS,
        METALNESS,
        DEPTH,
        POSITION,
        UV,

        DIFFUSE_LIGHTING,
        SPECULAR_LIGHTING,
        SHADOWMAP
    };

    struct FrameContext {
        int frameIndex{};
        float frameTime{};
        VkCommandBuffer commandBuffer{};
        Camera& camera;
        Scene& currentScene;
        DebugView debugView = DebugView::NONE;
    };
}

#endif //FRAMECONTEXT_H
