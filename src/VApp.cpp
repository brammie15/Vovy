#include "VApp.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_ALIGNED_GENTYPES
#include <chrono>
#include <glm/glm.hpp>

#include <functional>
#include <iostream>
#include <thread>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include "Descriptors/DescriptorWriter.h"
#include "GLFW/glfw3.h"
#include "Rendering/RenderSystems/ImguiRenderSystem.h"
#include "Rendering/RenderSystems/LineRenderSystem.h"
#include "Scene/Lights/DirectionalLight.h"
#include "Utils/BezierCurves.h"
#include "Utils/Camera.h"
#include "Utils/DeltaTime.h"
#include "Utils/FrameContext.h"
#include "Utils/LineManager.h"

VApp::VApp() {
    m_sigmaVanniScene = std::make_unique<vov::Scene>("SigmaVanniScene");
    m_sponzaScene = std::make_unique<vov::Scene>("SponzaScene");
    m_vikingRoomScene = std::make_unique<vov::Scene>("VikingRoomScene");
    m_bistroScene = std::make_unique<vov::Scene>("Bistro");
    m_flightHelmetScene = std::make_unique<vov::Scene>("FligtHelmetScene");
    m_chessScene = std::make_unique<vov::Scene>("ChessScene");

    m_scenes.emplace_back(m_sigmaVanniScene.get());
    m_scenes.emplace_back(m_sponzaScene.get());
    m_scenes.emplace_back(m_vikingRoomScene.get());
    m_scenes.emplace_back(m_bistroScene.get());
    m_scenes.emplace_back(m_flightHelmetScene.get());
    m_scenes.emplace_back(m_chessScene.get());


    loadGameObjects();
    m_currentScene = m_flightHelmetScene.get();
    m_currentScene->SceneLoad();

    m_renderer.SetResizeCallback([&] (const VkExtent2D newSize) {
        this->ResizeScreen(newSize);
    });

    m_hdrEnvironment = std::make_unique<vov::HDRI>(m_device);
    m_hdrEnvironment->LoadHDR("resources/circus_arena_4k.hdr");
    m_hdrEnvironment->CreateCubeMap();
    m_hdrEnvironment->CreateDiffuseIrradianceMap();

    m_depthPrePass = std::make_unique<vov::DepthPrePass>(
        m_device
    );
    m_depthPrePass->Init(VK_FORMAT_D32_SFLOAT, vov::Swapchain::MAX_FRAMES_IN_FLIGHT);

    m_shadowPass = std::make_unique<vov::ShadowPass>(
        m_device,
        vov::Swapchain::MAX_FRAMES_IN_FLIGHT,
        VK_FORMAT_D32_SFLOAT,
        m_renderer.getSwapchain().GetSwapChainExtent()
    );

    vov::GeometryPass::CreateInfo createInfo = {
        vov::Swapchain::MAX_FRAMES_IN_FLIGHT, m_currentScene, m_window.getExtent(), VK_FORMAT_D32_SFLOAT
    };

    m_geoPass = std::make_unique<vov::GeometryPass>(m_device, createInfo);

    //TODO: figure out format here;
    m_lightingPass = std::make_unique<vov::LightingPass>(m_device, vov::Swapchain::MAX_FRAMES_IN_FLIGHT, VK_FORMAT_R16G16B16A16_SFLOAT, m_renderer.getSwapchain().GetSwapChainExtent(), m_hdrEnvironment.get());

    m_blitPass = std::make_unique<vov::BlitPass>(
        m_device, vov::Swapchain::MAX_FRAMES_IN_FLIGHT, *m_lightingPass, m_renderer.getSwapchain()
    );

    m_linePass = std::make_unique<vov::LinePass>(
        m_device, vov::Swapchain::MAX_FRAMES_IN_FLIGHT, m_renderer.getSwapchain().GetSwapChainExtent()
    );

    m_imguiRenderSystem = std::make_unique<vov::ImguiRenderSystem>(
        m_device,
        VK_FORMAT_B8G8R8A8_SRGB, //TODO: this should really be a global
        static_cast<int>(m_window.getWidth()),
        static_cast<int>(m_window.getHeight())
    );

    m_camera.setAspectRatio(m_renderer.GetAspectRatio());

    m_camera.GetISO() = 1600.f;
    m_camera.GetAperture() = 0.7f;
    m_camera.GetShutterSpeed() = 1.f / 60.f;

    m_appGui = std::make_unique<AppGui>(m_imguiRenderSystem.get(), m_currentScene, &m_camera);
}

VApp::~VApp() = default;

void VApp::run() {
    while (!m_window.ShouldClose()) {
        vov::DeltaTime::GetInstance().Update();
        const double currentFps = 1.0 / vov::DeltaTime::GetInstance().GetDeltaTime();
        m_fpsAccumulated += currentFps;
        m_fpsFrameCount++;

        if (m_fpsFrameCount >= 100) {
            m_avgFps = m_fpsAccumulated / m_fpsFrameCount;
            m_fpsAccumulated = 0.0;
            m_fpsFrameCount = 0;
        }

        m_window.PollInput();
        m_imguiRenderSystem->beginFrame();

        this->imGui();
        if (!m_currentScene->getGameObjects().empty() && m_selectedTransform) {
            m_imguiRenderSystem->drawGizmos(&m_camera, m_selectedTransform, "Maintransform");
        }

        glm::vec3 worldDir = m_currentScene->GetDirectionalLight().GetDirection() * -1.0f;
        auto viewRot = glm::mat3(m_camera.GetViewMatrix());
        glm::vec3 camRelativeDir = viewRot * worldDir;

        m_imguiRenderSystem->drawDirection(&m_camera, camRelativeDir, "DirectionalLight");
        glm::vec3 updatedWorldDir = glm::transpose(viewRot) * camRelativeDir;
        updatedWorldDir = glm::normalize(updatedWorldDir);
        m_currentScene->GetDirectionalLight().SetDirection(updatedWorldDir * -1.0f);

        if (m_currentDebugViewMode != vov::DebugView::FULLSCREEN_SHADOW && m_RenderBoundingBoxes) {
            m_currentScene->getGameObjects()[0]->model->RenderBox();
        }

        // vov::LineManager::GetInstance().DrawWireSphere(glm::vec3(0, 10, 0), 5, 32);

        m_imguiRenderSystem->endFrame();

        if (m_window.isKeyPressed(GLFW_KEY_GRAVE_ACCENT)) {
            if (m_window.isCursorLocked()) {
                m_window.UnlockCursor();
            } else {
                m_window.LockCursor();
            }
        }

        if (m_window.isKeyPressed(GLFW_KEY_F1)) {
            m_renderImgui = !m_renderImgui;
        }


        m_camera.Update(static_cast<float>(vov::DeltaTime::GetInstance().GetDeltaTime()));
        m_currentScene->GetDirectionalLight().CalculateSceneBoundsMatricies(m_currentScene);

        if (const auto commandBuffer = m_renderer.BeginFrame()) {
            const int frameIndex = m_renderer.GetFrameIndex();
            vov::FrameContext frameContext{
                frameIndex,
                static_cast<float>(vov::DeltaTime::GetInstance().GetDeltaTime()),
                commandBuffer,
                m_camera,
                *m_currentScene,
                m_currentDebugViewMode
            };

            auto& depthImage = m_renderer.GetCurrentDepthImage();
            m_depthPrePass->Record(frameContext, depthImage);

            m_shadowPass->Record(frameContext);

            m_geoPass->Record(frameContext, depthImage);

            depthImage.TransitionImageLayout(
                commandBuffer,
                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
            );


            m_linePass->Record(frameContext, commandBuffer, frameIndex, depthImage);

            depthImage.TransitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

            m_lightingPass->UpdateDescriptors(
                frameIndex,
                m_geoPass->GetAlbedo(frameIndex),
                m_geoPass->GetNormal(frameIndex),
                m_geoPass->GetSpecualar(frameIndex),
                m_geoPass->GetWorldPos(frameIndex),
                depthImage,
                m_shadowPass->GetDepthImage(frameIndex)
            );

            m_lightingPass->Record(frameContext, commandBuffer, frameIndex, *m_geoPass, *m_hdrEnvironment, *m_shadowPass, *m_currentScene);

            m_blitPass->UpdateDescriptor(frameIndex, m_lightingPass->GetImage(frameIndex), m_linePass->GetImage(frameIndex));

            depthImage.TransitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

            //Swapchain render pass
            m_renderer.beginSwapChainRenderPass(commandBuffer); {
                m_blitPass->Record(frameContext, commandBuffer, frameIndex, m_renderer.getSwapchain());
                if (m_renderImgui) {
                    m_imguiRenderSystem->renderImgui(commandBuffer);
                }
            }
            m_renderer.endSwapChainRenderPass(commandBuffer);

            m_renderer.endFrame();
        }

        vov::LineManager::GetInstance().clear();
        // FPS limiter
        const auto sleepTime = vov::DeltaTime::GetInstance().SleepDuration();
        if (sleepTime > std::chrono::nanoseconds(0)) {
            std::this_thread::sleep_for(sleepTime);
        }
    }
    vkDeviceWaitIdle(m_device.device());
}

void VApp::imGui() {
    m_appGui->Render(m_avgFps, WIDTH, HEIGHT, m_selectedTransform, m_currentDebugViewMode,m_scenes);
}

void VApp::ResizeScreen(const VkExtent2D newSize) {
    m_depthPrePass->Resize(newSize);
    m_shadowPass->Resize(newSize);
    m_geoPass->Resize(newSize);
    m_lightingPass->Resize(newSize);
    m_blitPass->Resize(newSize, *m_lightingPass);
    m_linePass->Resize(newSize);

    m_camera.setAspectRatio(static_cast<float>(newSize.width) / static_cast<float>(newSize.height));
}

void VApp::loadGameObjects() {
    m_sigmaVanniScene->setSceneLoadFunction([&] (vov::Scene* scene) {
        auto sigmaVanni = vov::GameObject::LoadModelFromDisk(m_device, "resources/sigmavanni/SigmaVanni.gltf");
        scene->addGameObject(std::move(sigmaVanni));

        auto& directionalLight = scene->GetDirectionalLight();
        // directionalLight.setDirection(glm::vec3(0.0f, -0.5f, 0.5f));
        directionalLight.setColor(glm::vec3(1.0f, 1.0f, 1.0f));
        directionalLight.setIntensity(1.0f);
    });

    m_sponzaScene->setSceneLoadFunction([&] (vov::Scene* scene) {
        auto sponza = vov::GameObject::LoadModelFromDisk(m_device, "resources/Sponza/Sponza.gltf");
        scene->addGameObject(std::move(sponza));
    });

    m_vikingRoomScene->setSceneLoadFunction([&] (vov::Scene* scene) {
        auto vikingRoom = vov::GameObject::LoadModelFromDisk(m_device, "resources/viking_room.obj");
        scene->addGameObject(std::move(vikingRoom));
    });

    m_bistroScene->setSceneLoadFunction([&] (vov::Scene* scene) {
        // auto bistro = vov::GameObject::LoadModelFromDisk(m_device, "resources/PWP/PWP.gltf");
        auto bistro = vov::GameObject::LoadModelFromDisk(m_device, "resources/Bistro_v5_2/BistroExterior.fbx");
        // auto bistro = vov::GameObject::LoadModelFromDisk(m_device, "resources/testdds.fbx");
        bistro->transform.SetWorldScale(0.1f, 0.1f, 0.1f);
        scene->addGameObject(std::move(bistro));
    });

    m_flightHelmetScene->setSceneLoadFunction([&] (vov::Scene* scene) {
        auto flightHelmet = vov::GameObject::LoadModelFromDisk(m_device, "resources/FlightHelmet/FlightHelmet.gltf");
        scene->addGameObject(std::move(flightHelmet));
    });

    m_chessScene->setSceneLoadFunction([&] (vov::Scene* scene) {
        //From GLTF example models
        auto chess = vov::GameObject::LoadModelFromDisk(m_device, "resources/ABeautifulGame/glTF/ABeautifulGame.gltf");
        scene->addGameObject(std::move(chess));
    });
}
