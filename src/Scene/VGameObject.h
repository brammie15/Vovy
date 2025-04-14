#ifndef VGAMEOBJECT_H
#define VGAMEOBJECT_H
#include <cstdint>

#include "VMesh.h"
#include <memory>

#include "Transform.h"
#include "VModel.h"

class VGameObject {
public:

    //TODO: ask if dirty or not
    static std::unique_ptr<VGameObject> createGameObject() {
        static uint32_t id = 0;
        return std::make_unique<VGameObject>(id++);
    }

    VGameObject(const VGameObject&) = delete;
    VGameObject& operator=(const VGameObject&) = delete;
    VGameObject(VGameObject&&) = default;
    VGameObject& operator=(VGameObject&&) = default;

    [[nodiscard]] uint32_t getId() const { return m_id; }

    std::shared_ptr<VModel> model{};
    glm::vec3 color{};

    Transform transform{};
    explicit VGameObject(uint32_t id): m_id(id) {}
private:
    uint32_t m_id;
};

#endif //VGAMEOBJECT_H
