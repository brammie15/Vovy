#include "DirectionalLight.h"

#include <vector>

#include "Scene/Model.h"
#include "Scene/Scene.h"
#include "Utils/AABB.h"

namespace vov {

class Model;
class Scene;

DirectionalLight::DirectionalLight()
    : m_direction(glm::normalize(glm::vec3(-0.577f, 0.577f, -0.577f))),
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

glm::mat4 DirectionalLight::GetViewMatrix() {
    // updateLightSpaceMatrix();
    return m_lightView;
}

glm::mat4 DirectionalLight::GetProjectionMatrix() {
    // updateLightSpaceMatrix();
    return m_lightProjection;
}

void DirectionalLight::CalculateSceneBoundsMatricies(Scene* scene) {
    Model* model = scene->getGameObjects().front()->model.get();
    const glm::vec3 center = model->GetBoundingBox().GetCenter();
    const glm::vec3 direction = glm::normalize(m_direction) * -1.0f; // Invert direction for light space
    const AABB& boundingBox = model->GetBoundingBox();

    const std::vector<glm::vec3> corners = {
        {boundingBox.min.x, boundingBox.min.y, boundingBox.min.z},
        {boundingBox.max.x, boundingBox.min.y, boundingBox.min.z},
        {boundingBox.min.x, boundingBox.max.y, boundingBox.min.z},
        {boundingBox.max.x, boundingBox.max.y, boundingBox.min.z},
        {boundingBox.min.x, boundingBox.min.y, boundingBox.max.z},
        {boundingBox.max.x, boundingBox.min.y, boundingBox.max.z},
        {boundingBox.min.x, boundingBox.max.y, boundingBox.max.z},
        {boundingBox.max.x, boundingBox.max.y, boundingBox.max.z}
    };

    float minProj = FLT_MAX;
    float maxProj = -FLT_MAX;

    for (const glm::vec3& corner : corners) {
        const float proj = glm::dot(corner, direction);
        minProj = glm::min(minProj, proj);
        maxProj = glm::max(maxProj, proj);
    }

    const float distance = maxProj - glm::dot(center, direction);
    const glm::vec3 lightPos = center + direction * distance;

    const glm::vec3 up = glm::abs(glm::dot(direction, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.999f
                             ? glm::vec3(0.0f, 0.0f, 1.0f)
                             : glm::vec3(0.0f, 1.0f, 0.0f);

    m_lightView = glm::lookAtLH(lightPos, lightPos + direction, up);

    auto minLightSpace = glm::vec3(FLT_MAX);
    auto maxLightSpace = glm::vec3(-FLT_MAX);

    for (const glm::vec3& corner : corners) {
        const auto transformedCorner = glm::vec3(m_lightView * glm::vec4(corner, 1.0f));
        maxLightSpace = glm::max(maxLightSpace, transformedCorner);
        minLightSpace = glm::min(minLightSpace, transformedCorner);
    }

    const float nearZ = 0.0f;
    float farZ = maxLightSpace.z - minLightSpace.z;

    // farZ = 10;
    m_lightProjection = glm::orthoZO(
        minLightSpace.x, maxLightSpace.x,
        minLightSpace.y, maxLightSpace.y,
        nearZ, farZ
    );

    // m_lightProjection = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 50.0f);
}

void DirectionalLight::updateLightSpaceMatrix() {
    // const float nearPlane = 0.1f;
    // const float farPlane = 100.0f;
    //
    // m_lightProjection = glm::orthoZO(
    //     -m_orthoSize, m_orthoSize,
    //     -m_orthoSize, m_orthoSize,
    //     nearPlane, farPlane
    // );
    //
    // m_lightProjection[1][1] *= -1; // Invert Y axis for Vulkan
    //
    // m_lightView = glm::lookAt(
    //     -glm::normalize(m_direction) * m_lightDistance + m_lookAtPoint,
    //     m_lookAtPoint,
    //     glm::vec3(0.0f, 1.0f, 0.0f)
    // );
    //
    // m_lightSpaceMatrix = m_lightProjection * m_lightView;
}

} // namespace vov
