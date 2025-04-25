#include "DirectionalLight.h"

namespace vov {
    DirectionalLight::DirectionalLight(): direction(0.0f, -1.0f, 0.0f),
                                          color(1.0f, 1.0f, 1.0f),
                                          intensity(1.0f),
                                          shadowEnabled(false),
                                          shadowMapSize(2048), lightSpaceMatrix{} {
    }

    void DirectionalLight::setDirection(const glm::vec3& newDirection) {
        direction = glm::normalize(newDirection);
        updateLightSpaceMatrix();
    }

    DirectionalLight::UniformBufferObject DirectionalLight::getUBO() const {
        UniformBufferObject ubo{};
        ubo.direction = glm::vec4(direction, 0.0f);
        ubo.color = glm::vec4(color, intensity);
        ubo.lightSpaceMatrix = lightSpaceMatrix;
        return ubo;
    }

    void DirectionalLight::updateLightSpaceMatrix() {
        const float orthoSize = 25.0f;
        const float nearPlane = 0.1f;
        const float farPlane = 100.0f;

        glm::mat4 lightProjection = glm::ortho(
            -orthoSize, orthoSize,
            -orthoSize, orthoSize,
            nearPlane, farPlane);

        glm::mat4 lightView = glm::lookAt(
            -direction * 20.0f, // Position the light far away in the opposite direction
            glm::vec3(0.0f), // Look at the origin
            glm::vec3(0.0f, 1.0f, 0.0f)); // Up vector

        lightSpaceMatrix = lightProjection * lightView;
    }
}
