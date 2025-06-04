#ifndef VCAMERA_H
#define VCAMERA_H

#include <glm/glm.hpp>

#include "AABB.h"

namespace vov {
    class Camera {
    public:

        struct Frustum {
            glm::vec4 planes[6]; // left, right, bottom, top, near, far

            void update(const glm::mat4& vpMatrix) {
                // Extract frustum planes from VP matrix (row-major, GLM style)
                glm::mat4 m = vpMatrix;

                // Left
                planes[0] = glm::vec4(
                    m[0][3] + m[0][0],
                    m[1][3] + m[1][0],
                    m[2][3] + m[2][0],
                    m[3][3] + m[3][0]
                );
                // Right
                planes[1] = glm::vec4(
                    m[0][3] - m[0][0],
                    m[1][3] - m[1][0],
                    m[2][3] - m[2][0],
                    m[3][3] - m[3][0]
                );
                // Bottom
                planes[2] = glm::vec4(
                    m[0][3] + m[0][1],
                    m[1][3] + m[1][1],
                    m[2][3] + m[2][1],
                    m[3][3] + m[3][1]
                );
                // Top
                planes[3] = glm::vec4(
                    m[0][3] - m[0][1],
                    m[1][3] - m[1][1],
                    m[2][3] - m[2][1],
                    m[3][3] - m[3][1]
                );
                // Near
                planes[4] = glm::vec4(
                    m[0][3] + m[0][2],
                    m[1][3] + m[1][2],
                    m[2][3] + m[2][2],
                    m[3][3] + m[3][2]
                );
                // Far
                planes[5] = glm::vec4(
                    m[0][3] - m[0][2],
                    m[1][3] - m[1][2],
                    m[2][3] - m[2][2],
                    m[3][3] - m[3][2]
                );

                // Normalize all planes
                for (auto& plane : planes) {
                    float length = glm::length(glm::vec3(plane));
                    plane /= length;
                }
            }

            // Fast AABB-Frustum test
            [[nodiscard]] bool isBoxVisible(const AABB& box) const {
                for (const auto& plane : planes) {
                    glm::vec3 positive = box.min;
                    if (plane.x >= 0) positive.x = box.max.x;
                    if (plane.y >= 0) positive.y = box.max.y;
                    if (plane.z >= 0) positive.z = box.max.z;

                    if (glm::dot(glm::vec3(plane), positive) + plane.w < 0) {
                        return false;
                    }
                }
                return true;
            }
        };

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
        void ClearTarget();
        void Target(const glm::vec3& target);
        [[nodiscard]] glm::vec3 GetTarget() const { return m_target; }
        [[nodiscard]] bool IsTargetting() const { return m_useTarget; }

        [[nodiscard]] const Frustum& GetFrustum() const { return m_frustum; }

        [[nodiscard]] glm::vec3 GetPosition() const { return m_position; }
        [[nodiscard]] glm::vec3 GetForward() const { return m_forward; }
        [[nodiscard]] glm::vec3 GetUp() const { return m_up; }
        [[nodiscard]] glm::vec3 GetRight() const { return m_right; }

        void SetMovementSpeed(float speed) { m_movementSpeed = speed; }
        [[nodiscard]] float GetMovementSpeed() const { return m_movementSpeed; }


        //Kinda meh but bite me >:)
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

        float m_scrollSpeed = 2.0f;

        Frustum m_frustum{};

        float m_movementSpeed{2.0f};

    };
}

#endif //VCAMERA_H
