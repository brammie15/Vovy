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
        SHADOWMAP,
        FULLSCREEN_SHADOW,

        VISUALISE_DEFERRED_LAYERS,

        COUNT
    };

    inline std::string DebugViewToString(DebugView view) {
        switch (view) {
            case DebugView::NONE: return "None";
            case DebugView::ALBEDO: return "Albedo";
            case DebugView::NORMAL: return "Normal";
            case DebugView::SPECULAR: return "Specular";
            case DebugView::ROUGHNESS: return "Roughness";
            case DebugView::METALNESS: return "Metalness";
            case DebugView::DEPTH: return "Depth";
            case DebugView::POSITION: return "Position";
            case DebugView::UV: return "UV";
            case DebugView::DIFFUSE_LIGHTING: return "Diffuse Lighting";
            case DebugView::SPECULAR_LIGHTING: return "Specular Lighting";
            case DebugView::SHADOWMAP: return "Shadowmap";
            case DebugView::FULLSCREEN_SHADOW: return "Fullscreen Shadow";
            case DebugView::VISUALISE_DEFERRED_LAYERS: return "Visualise Deferred Layers";
            default: return "Unknown";
        }
    }

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
