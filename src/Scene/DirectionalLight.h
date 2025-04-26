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
            glm::mat4 lightProjection;
            glm::mat4 lightView;
            glm::mat4 viewProjection;

            glm::mat4 lightSpaceMatrix;
        };

        DirectionalLight();

        void setDirection(const glm::vec3& newDirection);


        void setColor(const glm::vec3& newColor) {
            m_color = newColor;
        }

        void setIntensity(float newIntensity) {
            m_intensity = newIntensity;
        }

        void enableShadows(bool enable) {
            m_shadowEnabled = enable;
        }

        void setShadowMapSize(uint32_t size) {
            m_shadowMapSize = size;
        }

        [[nodiscard]] UniformBufferObject getUBO() const;

        [[nodiscard]] const glm::mat4& getLightSpaceMatrix();

        [[nodiscard]] bool isShadowEnabled() const { return m_shadowEnabled; }
        [[nodiscard]] uint32_t getShadowMapSize() const { return m_shadowMapSize; }

        [[nodiscard]] glm::mat4 getLightProjection();
        [[nodiscard]] glm::mat4 getLightView();
        glm::vec3& getDirection() { return m_direction; }

    private:
        void updateLightSpaceMatrix();

        glm::vec3 m_direction;
        glm::vec3 m_color;
        float m_intensity;
        bool m_shadowEnabled;
        uint32_t m_shadowMapSize;
        glm::mat4 m_lightSpaceMatrix;

        float m_orthoSize = 10.0f;
        float m_lightDistance = 30.0f;
        glm::vec3 m_lookAtPoint{ 0.0f, 0.0f, 0.0f };

        glm::mat4 m_lightProjection{};
        glm::mat4 m_lightView{};

        bool m_viewDirty{ true };
        bool m_projDirty{ true };
    };
}


#endif //DIRECTIONALLIGHT_H
