#include "CookingStation/Math/Geometry.h"
#include <cmath> 

void Plane::Normalize() {
    float mag = glm::length(normal);

    if (mag > 0.000001f) {
        normal /= mag;
        distance /= mag;
    }
}

Frustum ExtractFrustum(const glm::mat4& viewProj) {
    Frustum frustum;

    frustum.leftFace.normal.x = viewProj[0][3] + viewProj[0][0];
    frustum.leftFace.normal.y = viewProj[1][3] + viewProj[1][0];
    frustum.leftFace.normal.z = viewProj[2][3] + viewProj[2][0];
    frustum.leftFace.distance = viewProj[3][3] + viewProj[3][0];

    frustum.rightFace.normal.x = viewProj[0][3] - viewProj[0][0];
    frustum.rightFace.normal.y = viewProj[1][3] - viewProj[1][0];
    frustum.rightFace.normal.z = viewProj[2][3] - viewProj[2][0];
    frustum.rightFace.distance = viewProj[3][3] - viewProj[3][0];

    frustum.bottomFace.normal.x = viewProj[0][3] + viewProj[0][1];
    frustum.bottomFace.normal.y = viewProj[1][3] + viewProj[1][1];
    frustum.bottomFace.normal.z = viewProj[2][3] + viewProj[2][1];
    frustum.bottomFace.distance = viewProj[3][3] + viewProj[3][1];

    frustum.topFace.normal.x = viewProj[0][3] - viewProj[0][1];
    frustum.topFace.normal.y = viewProj[1][3] - viewProj[1][1];
    frustum.topFace.normal.z = viewProj[2][3] - viewProj[2][1];
    frustum.topFace.distance = viewProj[3][3] - viewProj[3][1];

    frustum.nearFace.normal.x = viewProj[0][3] + viewProj[0][2];
    frustum.nearFace.normal.y = viewProj[1][3] + viewProj[1][2];
    frustum.nearFace.normal.z = viewProj[2][3] + viewProj[2][2];
    frustum.nearFace.distance = viewProj[3][3] + viewProj[3][2];

    frustum.farFace.normal.x = viewProj[0][3] - viewProj[0][2];
    frustum.farFace.normal.y = viewProj[1][3] - viewProj[1][2];
    frustum.farFace.normal.z = viewProj[2][3] - viewProj[2][2];
    frustum.farFace.distance = viewProj[3][3] - viewProj[3][2];

    frustum.leftFace.Normalize();
    frustum.rightFace.Normalize();
    frustum.bottomFace.Normalize();
    frustum.topFace.Normalize();
    frustum.nearFace.Normalize();
    frustum.farFace.Normalize();

    return frustum;
}

bool IsOnFrustum(const Frustum& camFrustum, const AABB& aabb) {

    const Plane planes[6] = {
        camFrustum.topFace,
        camFrustum.bottomFace,
        camFrustum.rightFace,
        camFrustum.leftFace,
        camFrustum.farFace,
        camFrustum.nearFace
    };

    for (int i = 0; i < 6; ++i) {

        const float r = aabb.extents.x * std::abs(planes[i].normal.x) +
            aabb.extents.y * std::abs(planes[i].normal.y) +
            aabb.extents.z * std::abs(planes[i].normal.z);

        const float d = glm::dot(planes[i].normal, aabb.center) + planes[i].distance;

        if (d < -r) {
            return false;
        }
    }

    return true;
}
