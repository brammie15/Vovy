#ifndef POINTLIGHT_H
#define POINTLIGHT_H

#include <glm/glm.hpp>

#include "../Transform.h"

namespace vov {
    class PointLight {
    public:
        struct alignas(16) PointLightData {
            glm::vec3 position;
            float padding; // padding to align next vec3
            glm::vec3 color;
            float intensity;
            float range;
            float padding3[2]; // padding to align struct size to 16
        };

        explicit PointLight() = default;
        explicit PointLight(const glm::vec3& position);

        void setPosition(const glm::vec3& position) { m_transform.SetWorldPosition(position); }
        void setColor(const glm::vec3& color) { m_color = color; }
        void setIntensity(float intensity) { m_intensity = intensity; }
        void setRange(float range) { m_range = range; }

        [[nodiscard]] const glm::vec3& getPosition() { return m_transform.GetWorldPosition(); }
        [[nodiscard]] const Transform& getTransform() const { return m_transform; }
        [[nodiscard]] Transform& getTransform() { return m_transform; }
        [[nodiscard]] const glm::vec3& getColor() const { return m_color; }
        [[nodiscard]] float getIntensity() const { return m_intensity; }
        [[nodiscard]] float getRange() const { return m_range; }

        [[nodiscard]] PointLightData getPointLightData() {
            PointLightData data{};
            data.position = m_transform.GetWorldPosition();
            data.color = m_color;
            data.intensity = m_intensity;
            data.range = m_range;
            return data;
        }

        void RenderImGui();
        Transform* GetTransform() { return &m_transform; }

    private:
        Transform m_transform;
        glm::vec3 m_color{1.0f, 1.0f, 1.0f};
        float m_intensity{ 1.0f };
        float m_range{ 10.0f };
    };
}

#endif //POINTLIGHT_H
