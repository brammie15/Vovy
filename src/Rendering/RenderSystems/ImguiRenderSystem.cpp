#include "ImguiRenderSystem.h"

#include <imgui_internal.h>
#include <backends/imgui_impl_glfw.h>
#include <stdexcept>
#include <ImGuizmo.h>
#include <glm/gtc/type_ptr.hpp>
#include "Rendering/VSwapchain.h"


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
    // ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

    setupDockspace();
}

static ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
static ImGuizmo::MODE currentGizmoMode = ImGuizmo::WORLD;

void ImguiRenderSystem::endFrame() {
    ImGui::Render();
    m_frameStarted = false;
}

void ImguiRenderSystem::drawGizmos(VCamera* camera, Transform* transform, const std::string& id) {
    if (transform == nullptr) {
        return;
    }
    if (!m_frameStarted) {
        ImGuizmo::BeginFrame();
    }

    ImGuizmo::DrawGrid(
      glm::value_ptr(camera->GetViewMatrix()),
      glm::value_ptr(camera->GetProjectionMatrix()),
      glm::value_ptr(glm::mat4(1.f)),
      1.f
  );

    ImGui::Begin("Gizmos");

    if (ImGui::RadioButton("Translate", currentGizmoOperation == ImGuizmo::TRANSLATE))
        currentGizmoOperation = ImGuizmo::TRANSLATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", currentGizmoOperation == ImGuizmo::ROTATE))
        currentGizmoOperation = ImGuizmo::ROTATE;
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", currentGizmoOperation == ImGuizmo::SCALE))
        currentGizmoOperation = ImGuizmo::SCALE;


    if (ImGui::RadioButton("World", currentGizmoMode == ImGuizmo::WORLD))
        currentGizmoMode = ImGuizmo::WORLD;
    ImGui::SameLine();
    if (ImGui::RadioButton("Local", currentGizmoMode == ImGuizmo::LOCAL))
        currentGizmoMode = ImGuizmo::LOCAL;


    ImGui::End();

    ImGuizmo::SetRect(0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);

    glm::mat4 objectMatrix = transform->GetWorldMatrix();
    ImGuizmo::PushID(id.c_str());
    ImGuizmo::Manipulate(
        glm::value_ptr(camera->GetViewMatrix()),
        glm::value_ptr(camera->GetProjectionMatrix()),
        currentGizmoOperation,
        currentGizmoMode,
        glm::value_ptr(objectMatrix)
    );
    ImGuizmo::PopID();

    transform->SetWorldMatrix(objectMatrix);
}

void ImguiRenderSystem::drawGizmos(VCamera* camera, glm::vec3& position, const std::string& id) {
    if (!m_frameStarted) {
        ImGuizmo::BeginFrame();
    }

    ImGuizmo::SetRect(0, 0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y);

    ImGuizmo::PushID(id.c_str());
    glm::mat4 objectMatrix = glm::translate(glm::mat4(1.f), position);
    ImGuizmo::Manipulate(
        glm::value_ptr(camera->GetViewMatrix()),
        glm::value_ptr(camera->GetProjectionMatrix()),
        currentGizmoOperation,
        currentGizmoMode,
        glm::value_ptr(objectMatrix)
    );
    ImGuizmo::PopID();

    position = glm::vec3(objectMatrix[3]);
}

void ImguiRenderSystem::setupDockspace() {
    ImGuiIO& io = ImGui::GetIO();
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
                                    ImGuiWindowFlags_NoTitleBar |
                                    ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNavFocus |
                                    ImGuiWindowFlags_NoBackground;

    // Get the menu bar height from ImGui's current style
    float menuBarHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeight(); // This is roughly the height of the menubar

    // Adjust the dockspace position and size to avoid overlap with the menubar
    ImVec2 newSize = ImVec2(viewport->Size.x, viewport->Size.y - menuBarHeight);
    ImVec2 newPos = ImVec2(viewport->Pos.x, viewport->Pos.y + menuBarHeight);

    ImGui::SetNextWindowPos(newPos);
    ImGui::SetNextWindowSize(newSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("FullscreenDockspace", nullptr, window_flags);
    ImGui::PopStyleVar(2);

    // Create the main dockspace
    ImGuiID dockspace_id = ImGui::GetID("MyDockspace");

    if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
        // Set up the main dockspace
        ImGui::DockBuilderRemoveNode(dockspace_id); // clear previous layout
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, newSize);

        // Split it into 5 areas
        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
        ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.20f, nullptr, &dock_main_id);
        ImGuiID dock_id_top = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Up, 0.15f, nullptr, &dock_main_id);
        ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.15f, nullptr, &dock_main_id);

        // The remaining dock_main_id is now the center. We do NOT dock anything to it.

        // Optionally dock windows:
        ImGui::DockBuilderDockWindow("Left Panel", dock_id_left);
        ImGui::DockBuilderDockWindow("Right Panel", dock_id_right);
        ImGui::DockBuilderDockWindow("Top Panel", dock_id_top);
        ImGui::DockBuilderDockWindow("Bottom Panel", dock_id_bottom);
        // Do not dock anything to dock_main_id

        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    ImGui::End();
}


void ImguiRenderSystem::initImgui(VkRenderPass renderPass, int width, int height) {
    m_descriptorPool =
            VDescriptorPool::Builder(m_device)
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            .setMaxSets(VSwapchain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VSwapchain::MAX_FRAMES_IN_FLIGHT)
            .build();

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui_ImplVulkan_InitInfo init_info = {};

    init_info.Instance = m_device.getInstance();
    init_info.PhysicalDevice = m_device.getPhysicalDevice();
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

    init_info.CheckVkResultFn = [] (VkResult err) {
        if (err != VK_SUCCESS) {
            throw std::runtime_error("ImGui Vulkan error");
        }
    };

    ImGui_ImplVulkan_Init(&init_info);
    ImGui_ImplGlfw_InitForVulkan(VWindow::gWindow, true);

    ImGui_ImplVulkan_CreateFontsTexture();
}
