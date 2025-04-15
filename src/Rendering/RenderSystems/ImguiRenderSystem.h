#ifndef IMGUIRENDERSYSTEM_H
#define IMGUIRENDERSYSTEM_H

#include "Core/VDevice.h"
#include "Scene/VGameObject.h"
#include "Rendering/VPipeline.h"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "Utils/VCamera.h"

class ImguiRenderSystem {
public:
    ImguiRenderSystem(VDevice& deviceRef, VkRenderPass renderPass, int width, int height);
    ~ImguiRenderSystem();

    void beginFrame();
    void endFrame();

    void renderImgui(VkCommandBuffer commandBuffer) {
        vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    }

    void drawGizmos(VCamera* camera, Transform* transform, const std::string& id);
    void drawGizmos(VCamera* camera, glm::vec3& position, const std::string& id);

private:
    void setupDockspace();
    void initImgui(VkRenderPass renderPass, int width, int height);

    VDevice& m_device;

    std::unique_ptr<VDescriptorPool> m_descriptorPool;

    bool m_frameStarted{false};
};

#endif //IMGUIRENDERSYSTEM_H
