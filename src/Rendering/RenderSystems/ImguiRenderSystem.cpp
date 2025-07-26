#include "ImguiRenderSystem.h"


#include <ImGuizmo.h>
#include <imgui_internal.h>
#include <stdexcept>
#include <backends/imgui_impl_glfw.h>
#include <glm/gtc/type_ptr.hpp>
#include "Rendering/Swapchain.h"


#include <glm/gtx/matrix_decompose.hpp>

namespace vov {
    ImguiRenderSystem::ImguiRenderSystem(Device& deviceRef, VkFormat colorFormat,  int width, int height): m_device{deviceRef} {
        initImgui(colorFormat, width, height);
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

    void ImguiRenderSystem::drawGizmos(const Camera* camera, Transform* transform, const std::string& id) const {
        if (transform == nullptr) {
            return;
        }
        if (!m_frameStarted) {
            ImGuizmo::BeginFrame();
        }

        ImGuizmo::DrawGrid(
            glm::value_ptr(camera->GetViewMatrix()),
            glm::value_ptr(camera->GetProjectionMatrix()),
            glm::value_ptr(transform->GetWorldMatrix()),
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

    void ImguiRenderSystem::drawGizmos(const Camera* camera, glm::vec3& position, const std::string& id) const {
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

    void ImguiRenderSystem::drawDirection(const Camera* camera, glm::vec3& dir, const std::string& id) const {

        // Get current quaternion rotation from the world matrix
        // Start a panel or window if needed (optional)
        ImGui::Begin("Direction");

        ImGuizmo::PushID(id.c_str());

        // Optional: define size and mode for gizmo
        constexpr float gizmoSize = 150.f; // pixels
        // constexpr int mode = imguiGizmo::modePanDolly|imguiGizmo::sphereAtOrigin;
        constexpr int mode = imguiGizmo::modeDirection;

        // Render the quaternion gizmo
        if (ImGui::gizmo3D("Rotation", dir, gizmoSize, mode)) {
        }

        ImGuizmo::PopID();
        ImGui::End();

    }


    void ImguiRenderSystem::setupDockspace() {
        ImGuiIO& io = ImGui::GetIO();
        const ImGuiViewport* viewport = ImGui::GetMainViewport();

        const ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking |
                                        ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoCollapse |
                                        ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoMove |
                                        ImGuiWindowFlags_NoBringToFrontOnFocus |
                                        ImGuiWindowFlags_NoNavFocus |
                                        ImGuiWindowFlags_NoBackground;

        // Get the menu bar height from ImGui's current style
        const float menuBarHeight = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeight(); // This is roughly the height of the menubar

        // Adjust the dockspace position and size to avoid overlap with the menubar
        const auto newSize = ImVec2(viewport->Size.x, viewport->Size.y - menuBarHeight);
        const auto newPos = ImVec2(viewport->Pos.x, viewport->Pos.y + menuBarHeight);

        ImGui::SetNextWindowPos(newPos);
        ImGui::SetNextWindowSize(newSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::Begin("FullscreenDockspace", nullptr, window_flags);
        ImGui::PopStyleVar(2);

        // Create the main dockspace
        const ImGuiID dockspace_id = ImGui::GetID("MyDockspace");

        if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
            // Set up the main dockspace
            ImGui::DockBuilderRemoveNode(dockspace_id); // clear previous layout
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, newSize);

            // Split it into 5 areas
            ImGuiID dock_main_id = dockspace_id;
            const ImGuiID dock_id_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.20f, nullptr, &dock_main_id);
            const ImGuiID dock_id_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.20f, nullptr, &dock_main_id);
            const ImGuiID dock_id_top = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Up, 0.15f, nullptr, &dock_main_id);
            const ImGuiID dock_id_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.15f, nullptr, &dock_main_id);

            // The remaining dock_main_id is now the center. We do NOT dock anything to it.

            // Optionally dock windows:
            ImGui::DockBuilderDockWindow("Left Panel", dock_id_left);
            ImGui::DockBuilderDockWindow("Right Panel", dock_id_right);
            ImGui::DockBuilderDockWindow("Top Panel", dock_id_top);
            ImGui::DockBuilderDockWindow("Bottom Panel", dock_id_bottom);
            // Do not dock anything to dock_main_id

            ImGui::DockBuilderFinish(dockspace_id);
        }

        constexpr ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        ImGui::End();
    }


    void ImguiRenderSystem::initImgui(VkFormat format, int width, int height) {
        m_descriptorPool =
                DescriptorPool::Builder(m_device)
                .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
                .setMaxSets(Swapchain::MAX_FRAMES_IN_FLIGHT)
                .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, Swapchain::MAX_FRAMES_IN_FLIGHT)
                .build();

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.IniFilename = "resources/imgui.ini";
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui_ImplVulkan_InitInfo init_info = {};

        init_info.Instance = m_device.getInstance();
        init_info.PhysicalDevice = m_device.getPhysicalDevice();
        init_info.Device = m_device.device();
        init_info.QueueFamily = m_device.FindPhysicalQueueFamilies().graphicsFamily;
        init_info.Queue = m_device.graphicsQueue();
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = m_descriptorPool->getDescriptorPool();
        init_info.MinImageCount = Swapchain::MAX_FRAMES_IN_FLIGHT;
        init_info.ImageCount = Swapchain::MAX_FRAMES_IN_FLIGHT;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = nullptr;
        init_info.UseDynamicRendering = true;
        init_info.ApiVersion = VK_API_VERSION_1_3;
        init_info.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;
        init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        init_info.PipelineRenderingCreateInfo.depthAttachmentFormat = VK_FORMAT_D32_SFLOAT;


        init_info.CheckVkResultFn = [] (const VkResult err) {
            if (err != VK_SUCCESS) {
                throw std::runtime_error("ImGui Vulkan error");
            }
        };

        ImGui_ImplVulkan_Init(&init_info);
        ImGui_ImplGlfw_InitForVulkan(Window::gWindow, true);

        // ImGui_ImplVulkan_CreateFontsTexture();
    }
}
