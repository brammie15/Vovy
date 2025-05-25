#ifndef VCAMERA_H
#define VCAMERA_H

#include <glm/glm.hpp>

namespace vov {
    class Camera {
    public:
        struct alignas(16) CameraSettings{
            glm::vec4 cameraPos{};
            float apeture{};
            float shutterSpeed{};
            float iso{};
            float _pad0;           // padding to align next vec3
        };


        explicit Camera(const glm::vec3& position, const glm::vec3& up);
        void Update(float deltaTime);

        void CalculateViewMatrix();
        void CalculateProjectionMatrix();

        [[nodiscard]] glm::mat4 GetViewMatrix() const { return m_viewMatrix; }
        [[nodiscard]] glm::mat4 GetProjectionMatrix() const { return m_projectionMatrix; }

        [[nodiscard]] glm::mat4 GetViewProjectionMatrix() const { return m_projectionMatrix * m_viewMatrix; }

        void setAspectRatio(float aspectRatio) {
            m_aspectRatio = aspectRatio;
            this->CalculateViewMatrix();
            this->CalculateProjectionMatrix();
        }
        glm::vec3 m_position{2.f, 0, 0 };

        void Translate(const glm::vec3& translation) {
            m_position += translation;
        }

        void SetTarget(const glm::vec3& target);
        [[nodiscard]] glm::vec3 GetTarget() const { return m_target; }
        void ClearTarget();
        bool IsTargetting() const { return m_useTarget; }
        void Target(const glm::vec3& target);


        float& GetISO() { return m_iso; }
        float& GetAperture() { return m_aperture; }
        float& GetShutterSpeed() { return m_shutterSpeed; }


    private:

        float fovAngle{90.f};
        float fov{tanf(glm::radians(fovAngle) / 2.f)};

        glm::vec3 m_forward {1, 0, 0};
        glm::vec3 m_up      {0, 1, 0};
        glm::vec3 m_right   {0, 0, 1};

        glm::vec3 m_target{ 0.0f, 0.0f, 0.0f };
        bool m_useTarget{ false };

        float totalPitch{ 0.0f };
        float totalYaw{ 0.0f };

        glm::mat4 m_viewMatrix{};
        glm::mat4 m_invMatrix{};

        glm::mat4 m_projectionMatrix{};

        float m_aspectRatio{};
        float m_cameraSpeed{};
        float m_cameraSensitivity{};

        float m_zNear{0.001f};
        float m_zFar{1000.0f};

        float m_iso{100.f};
        float m_aperture{2.8f};
        float m_shutterSpeed{1.f / 60.f};

    };
}

#endif //VCAMERA_H
