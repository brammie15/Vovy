#include "ImguiRenderSystem.h"

#include <stdexcept>

#include "VSwapchain.h"

ImguiRenderSystem::ImguiRenderSystem(VDevice& deviceRef, VkRenderPass renderPass, int width, int height): m_device{deviceRef} {
    initImgui(renderPass, width, height);
}

ImguiRenderSystem::~ImguiRenderSystem() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImguiRenderSystem::beginFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImguiRenderSystem::endFrame() {
    ImGui::Render();
}

void ImguiRenderSystem::initImgui(VkRenderPass renderPass, int width, int height) {
    m_descriptorPool =
       VDescriptorPool::Builder(m_device)
           .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
           .setMaxSets(VSwapchain::MAX_FRAMES_IN_FLIGHT)
           .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VSwapchain::MAX_FRAMES_IN_FLIGHT)
           .build();

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
    ImGui_ImplVulkan_InitInfo init_info = {};

    init_info.Instance = m_device.getInstance();
    init_info.PhysicalDevice = m_device.getPhyscialDevice();
    init_info.Device = m_device.device();
    init_info.QueueFamily = m_device.FindPhysicalQueueFamilies().graphicsFamily;
    init_info.Queue = m_device.graphicsQueue();
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = m_descriptorPool->getDescriptorPool();
    init_info.MinImageCount = VSwapchain::MAX_FRAMES_IN_FLIGHT;
    init_info.ImageCount = VSwapchain::MAX_FRAMES_IN_FLIGHT;
    init_info.Subpass = 1;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.RenderPass = renderPass;

    init_info.CheckVkResultFn = [](VkResult err) {
        if (err != VK_SUCCESS) {
            throw std::runtime_error("ImGui Vulkan error");
        }
    };

    ImGui_ImplVulkan_Init(&init_info);
    ImGui_ImplGlfw_InitForVulkan(VWindow::gWindow, true);

    ImGui_ImplVulkan_CreateFontsTexture();
}