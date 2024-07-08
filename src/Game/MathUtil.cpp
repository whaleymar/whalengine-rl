#include "MathUtil.h"

#include <cmath>

Vector2f closestOrdinalDirection(Vector2f vecf) {
    vecf = vecf.norm();
    const f32 invRootTwo = 1.0f / std::sqrt(2.0f);
    auto tryUnitDir = [vecf](Vector2f& closest, f32& minDegreesAway, Vector2f other) {
        f32 otherDP = vecf.dot(other);
        f32 degreesAway = std::abs(std::acos(otherDP));
        if (degreesAway < minDegreesAway) {
            closest = other;
            minDegreesAway = degreesAway;
        }
    };

    if (vecf.x() == 0) {
        if (vecf.y() == 0) {
            return {1.0, 0.0};
        } else {
            return Vector2f(0.0, sign(vecf.y()));
        }
    } else if (vecf.x() < 0) {
        if (vecf.y() == 0) {
            return {-1.0, 0};
        } else if (vecf.y() < 0) {
            // SW quadrant
            Vector2f closest = Vector2f::unitLeft;
            f32 degreesAway = std::abs(std::acos(vecf.dot(closest)));

            tryUnitDir(closest, degreesAway, Vector2f::unitDown);
            tryUnitDir(closest, degreesAway, Vector2f(-1, -1) * invRootTwo);
            return closest;
        } else {
            // NW quadrant
            Vector2f closest = Vector2f::unitLeft;
            f32 degreesAway = std::abs(std::acos(vecf.dot(closest)));

            tryUnitDir(closest, degreesAway, Vector2f::unitUp);
            tryUnitDir(closest, degreesAway, Vector2f(-1, 1) * invRootTwo);
            return closest;
        }

    } else {
        if (vecf.y() == 0) {
            return {1.0, 0};
        } else if (vecf.y() < 0) {
            // SE quadrant
            Vector2f closest = Vector2f::unitRight;
            f32 degreesAway = std::abs(std::acos(vecf.dot(closest)));

            tryUnitDir(closest, degreesAway, Vector2f::unitDown);
            tryUnitDir(closest, degreesAway, Vector2f(1, -1) * invRootTwo);
            return closest;

        } else {
            // NE quadrant
            Vector2f closest = Vector2f::unitRight;
            f32 degreesAway = std::abs(std::acos(vecf.dot(closest)));

            tryUnitDir(closest, degreesAway, Vector2f::unitUp);
            tryUnitDir(closest, degreesAway, Vector2f(1, 1) * invRootTwo);
            return closest;
        }
    }
}
