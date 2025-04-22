#ifndef VGAMEOBJECT_H
#define VGAMEOBJECT_H
#include <cstdint>

#include <memory>

#include "Transform.h"
#include "Model.h"

namespace vov {
    class GameObject {
    public:
        static std::unique_ptr<GameObject> createGameObject() {
            static uint32_t id = 0;
            return std::make_unique<GameObject>(id++);
        }

        GameObject(const GameObject&) = delete;
        GameObject& operator=(const GameObject&) = delete;
        GameObject(GameObject&&) = default;
        GameObject& operator=(GameObject&&) = default;

        [[nodiscard]] uint32_t getId() const { return m_id; }

        std::shared_ptr<Model> model{};
        glm::vec3 color{};

        static std::unique_ptr<GameObject> LoadModelFromDisk(Device& device, const std::string& filepath);

        Transform transform{};

        explicit GameObject(uint32_t id): m_id(id) {
        }

    private:
        uint32_t m_id;
    };
}


#endif //VGAMEOBJECT_H
