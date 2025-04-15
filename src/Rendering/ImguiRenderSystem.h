#ifndef IMGUIRENDERSYSTEM_H
#define IMGUIRENDERSYSTEM_H
#include "Core/VDevice.h"
#include "Scene/VGameObject.h"
#include "VPipeline.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "Utils/VCamera.h"


class ImguiRenderSystem {
public:
    ImguiRenderSystem(VDevice& deviceRef, VkRenderPass renderPass, int width, int height);
    ~ImguiRenderSystem();

    void beginFrame();
    void endFrame();

    void renderImgui(VkCommandBuffer commandBuffer) {
        // m_pipeline->bind(commandBuffer);
        vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    }

    void drawGizmos(VCamera* camera, Transform* transform);

private:
    void setupDockspace();
    void initImgui(VkRenderPass renderPass, int width, int height);

    VDevice& m_device;

    std::unique_ptr<VDescriptorPool> m_descriptorPool;
};



#endif //IMGUIRENDERSYSTEM_H
