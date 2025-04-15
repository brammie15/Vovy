#ifndef VCAMERA_H
#define VCAMERA_H

#include <glm/glm.hpp>


class VCamera {
public:
    explicit VCamera(const glm::vec3& position, const glm::vec3& up);
    void Update(float deltaTime);

    void CalculateViewMatrix();
    void CalculateProjectionMatrix();

    [[nodiscard]] glm::mat4 GetViewMatrix() const { return m_viewMatrix; }
    [[nodiscard]] glm::mat4 GetProjectionMatrix() const { return m_projectionMatrix; }

    [[nodiscard]] glm::mat4 GetViewProjectionMatrix() const { return m_projectionMatrix * m_viewMatrix; }

    void setAspectRatio(float aspectRatio) { m_aspectRatio = aspectRatio; }
    glm::vec3 m_position{2.f, 0, 0 };

    void Translate(const glm::vec3& translation) {
        m_position += translation;
    }

private:

    float fovAngle{90.f};
    float fov{tanf(glm::radians(fovAngle) / 2.f)};

    glm::vec3 m_forward {1, 0, 0};
    glm::vec3 m_up      {0, 1, 0};
    glm::vec3 m_right   {0, 0, 1};


    float totalPitch{ 0.0f };
    float totalYaw{ 0.0f };

    glm::mat4 m_viewMatrix{};
    glm::mat4 m_invMatrix{};

    glm::mat4 m_projectionMatrix{};

    float m_aspectRatio{};
    float m_cameraSpeed{};
    float m_cameraSensitivity{};

    float m_zNear{0.001f};
    float m_zFar{100.0f};
};


#endif //VCAMERA_H
