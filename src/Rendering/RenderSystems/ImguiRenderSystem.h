#ifndef IMGUIRENDERSYSTEM_H
#define IMGUIRENDERSYSTEM_H

#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "Core/Device.h"
#include "Rendering/Pipeline.h"
#include "Scene/GameObject.h"
#include "Utils/Camera.h"

namespace vov {
    class ImguiRenderSystem {
    public:
        ImguiRenderSystem(Device& deviceRef, VkFormat colorFormat, int width, int height);
        ~ImguiRenderSystem();

        void beginFrame();
        void endFrame();

        void renderImgui(VkCommandBuffer commandBuffer) {
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
        }

        void drawGizmos(Camera* camera, Transform* transform, const std::string& id);
        void drawGizmos(const Camera* camera, glm::vec3& position, const std::string& id) const;

    private:
        void setupDockspace();
        void initImgui(VkFormat format, int width, int height);

        Device& m_device;

        std::unique_ptr<DescriptorPool> m_descriptorPool;

        bool m_frameStarted{false};
    };
}

#endif //IMGUIRENDERSYSTEM_H
