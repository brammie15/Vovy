#ifndef VGAMEOBJECT_H
#define VGAMEOBJECT_H
#include <cstdint>

#include "VModel.h"
#include <memory>

struct Transform {
    glm::vec3 translation{1.f};
    glm::vec3 scale{1.f};
    glm::mat2 mat2() {
        return glm::mat2(translation, scale);
    }
};

class VGameObject {
public:

    //TODO: ask if dirty or not
    static VGameObject createGameObject() {
        static uint32_t id = 0;
        return VGameObject(id++);
    }

    VGameObject(const VGameObject&) = delete;
    VGameObject& operator=(const VGameObject&) = delete;
    VGameObject(VGameObject&&) = default;
    VGameObject& operator=(VGameObject&&) = default;

    [[nodiscard]] uint32_t getId() const { return m_id; }

    std::shared_ptr<VModel> model{};
    glm::vec3 color{};

    Transform transform{};
private:
    explicit VGameObject(uint32_t id): m_id(id) {}
    uint32_t m_id;
};

#endif //VGAMEOBJECT_H
