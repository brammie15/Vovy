#ifndef AABB_H
#define AABB_H

#include <glm/glm.hpp>

namespace vov {
    struct AABB {
        glm::vec3 min{};
        glm::vec3 max{};
        [[nodiscard]] glm::vec3 GetCenter() const;
    };

    bool RayAABBIntersection(
        const glm::vec3& rayOrigin,
        const glm::vec3& rayDir,
        const AABB& aabb,
        float& intersectionDistance);

    AABB TransformAABB(const AABB& localAABB, const glm::mat4& transform);
}


#endif //AABB_H
