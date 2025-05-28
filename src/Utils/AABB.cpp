#include "AABB.h"
#include "AABB.h"
#include "AABB.h"
#include "AABB.h"
#include "AABB.h"

#include <array>

glm::vec3 vov::AABB::GetCenter() const {
    return (min + max) * 0.5f;
}

bool vov::RayAABBIntersection(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const AABB& aabb, float& intersectionDistance) {
    const glm::vec3 dirfrac = 1.0f / rayDir;

    const float t1 = (aabb.min.x - rayOrigin.x) * dirfrac.x;
    const float t2 = (aabb.max.x - rayOrigin.x) * dirfrac.x;
    const float t3 = (aabb.min.y - rayOrigin.y) * dirfrac.y;
    const float t4 = (aabb.max.y - rayOrigin.y) * dirfrac.y;
    const float t5 = (aabb.min.z - rayOrigin.z) * dirfrac.z;
    const float t6 = (aabb.max.z - rayOrigin.z) * dirfrac.z;

    const float tmin = glm::max(glm::max(glm::min(t1, t2), glm::min(t3, t4)), glm::min(t5, t6));
    const float tmax = glm::min(glm::min(glm::max(t1, t2), glm::max(t3, t4)), glm::max(t5, t6));

    // If tmax < 0, ray is intersecting but the AABB is behind the origin
    if (tmax < 0) {
        intersectionDistance = tmax;
        return false;
    }

    // If tmin > tmax, ray doesn't intersect AABB
    if (tmin > tmax) {
        intersectionDistance = tmax;
        return false;
    }

    intersectionDistance = tmin;
    return true;
}

vov::AABB vov::TransformAABB(const AABB& localAABB, const glm::mat4& transform) {
    const std::array<glm::vec3, 8> corners = {
        glm::vec3(localAABB.min.x, localAABB.min.y, localAABB.min.z),
        glm::vec3(localAABB.min.x, localAABB.min.y, localAABB.max.z),
        glm::vec3(localAABB.min.x, localAABB.max.y, localAABB.min.z),
        glm::vec3(localAABB.min.x, localAABB.max.y, localAABB.max.z),
        glm::vec3(localAABB.max.x, localAABB.min.y, localAABB.min.z),
        glm::vec3(localAABB.max.x, localAABB.min.y, localAABB.max.z),
        glm::vec3(localAABB.max.x, localAABB.max.y, localAABB.min.z),
        glm::vec3(localAABB.max.x, localAABB.max.y, localAABB.max.z)
    };

    glm::vec3 min(FLT_MAX);
    glm::vec3 max(-FLT_MAX);

    for (const auto& corner : corners) {
        const auto transformed = glm::vec3(transform * glm::vec4(corner, 1.0f));

        min = glm::min(min, transformed);
        max = glm::max(max, transformed);
    }

    return {min, max};
}