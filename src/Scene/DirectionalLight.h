#ifndef DIRECTIONALLIGHT_H
#define DIRECTIONALLIGHT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace vov {
    class DirectionalLight {
    public:
        struct UniformBufferObject {
            glm::vec4 direction;
            glm::vec4 color;
            glm::mat4 lightSpaceMatrix;
        };

        DirectionalLight();

        void setDirection(const glm::vec3& newDirection);

        void setColor(const glm::vec3& newColor) {
            color = newColor;
        }

        void setIntensity(float newIntensity) {
            intensity = newIntensity;
        }

        void enableShadows(bool enable) {
            shadowEnabled = enable;
        }

        void setShadowMapSize(uint32_t size) {
            shadowMapSize = size;
        }

        UniformBufferObject getUBO() const;

        const glm::mat4& getLightSpaceMatrix() const {
            return lightSpaceMatrix;
        }

        bool isShadowEnabled() const { return shadowEnabled; }
        uint32_t getShadowMapSize() const { return shadowMapSize; }

    private:
        void updateLightSpaceMatrix();

        glm::vec3 direction;
        glm::vec3 color;
        float intensity;
        bool shadowEnabled;
        uint32_t shadowMapSize;
        glm::mat4 lightSpaceMatrix;
    };
}


#endif //DIRECTIONALLIGHT_H
