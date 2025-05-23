#include "DirectionalLight.h"

namespace vov {

DirectionalLight::DirectionalLight()
    : m_direction(glm::normalize(glm::vec3(1.0f, -1.0f, 0.0f))),
      m_color(1.0f, 1.0f, 1.0f),
      m_intensity(1.0f),
      m_shadowEnabled(false),
      m_shadowMapSize(2048),
      m_lightSpaceMatrix{}
{
}

void DirectionalLight::setDirection(const glm::vec3& newDirection) {
    m_direction = glm::normalize(newDirection);
    m_viewDirty = true;
}

DirectionalLight::UniformBufferObject DirectionalLight::GetUBO() const {
    UniformBufferObject ubo{};
    ubo.direction = glm::vec4(m_direction, 0.0f);
    ubo.color = glm::vec4(m_color, m_intensity);
    ubo.lightProjection = m_lightProjection;
    ubo.lightView = m_lightView;
    return ubo;
}

const glm::mat4& DirectionalLight::GetLightSpaceMatrix() {
    if (m_viewDirty || m_projDirty) {
        updateLightSpaceMatrix();
        m_viewDirty = false;
        m_projDirty = false;
    }
    return m_lightSpaceMatrix;
}

glm::mat4 DirectionalLight::GetLightProjection() {
    if (m_viewDirty) {
        updateLightSpaceMatrix();
        m_viewDirty = false;
    }
    return m_lightProjection;
}

glm::mat4 DirectionalLight::GetLightView() {
    if (m_viewDirty) {
        updateLightSpaceMatrix();
        m_viewDirty = false;
    }
    return m_lightView;
}

void DirectionalLight::updateLightSpaceMatrix() {
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;

    m_lightProjection = glm::orthoZO(
        -m_orthoSize, m_orthoSize,
        -m_orthoSize, m_orthoSize,
        nearPlane, farPlane
    );

    m_lightProjection[1][1] *= -1; // Invert Y axis for Vulkan

    m_lightView = glm::lookAt(
        -glm::normalize(m_direction) * m_lightDistance + m_lookAtPoint,
        m_lookAtPoint,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    m_lightSpaceMatrix = m_lightProjection * m_lightView;
}

} // namespace vov
