#ifndef DIRECTIONALLIGHT_H
#define DIRECTIONALLIGHT_H

#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


namespace vov {
    class Scene;

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

        [[nodiscard]] UniformBufferObject GetUBO() const;

        [[nodiscard]] const glm::mat4& GetLightSpaceMatrix();

        [[nodiscard]] bool IsShadowEnabled() const { return m_shadowEnabled; }
        [[nodiscard]] uint32_t GetShadowMapSize() const { return m_shadowMapSize; }

        [[nodiscard]] glm::mat4 GetLightProjection();
        [[nodiscard]] glm::mat4 GetLightView();

        glm::vec3& GetDirection() { return m_direction; }
        glm::vec3& GetColor(){ return m_color; }
        float& GetIntensity() { return m_intensity; }
        glm::mat4 GetViewMatrix();
        glm::mat4 GetProjectionMatrix();

        void CalculateSceneBoundsMatricies(Scene* scene);

    private:
        void updateLightSpaceMatrix();

        glm::vec3 m_direction;
        glm::vec3 m_color;
        float m_intensity;
        bool m_shadowEnabled;
        uint32_t m_shadowMapSize;
        glm::mat4 m_lightSpaceMatrix;

        float m_orthoSize = 5.0f;
        float m_lightDistance = 10.0f;
        glm::vec3 m_lookAtPoint{ 0.0f, 0.0f, 0.0f };

        glm::mat4 m_lightProjection{};
        glm::mat4 m_lightView{};

        bool m_viewDirty{ true };
        bool m_projDirty{ true };
    };
}


#endif //DIRECTIONALLIGHT_H
