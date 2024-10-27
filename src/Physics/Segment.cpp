#include "Physics/Segment.h"

#include "Physics/HitInfo.h"
#include "Shapes.h"
#include "Util/MathUtil.h"

namespace whal {

Segment::Segment(Vector2f delta_, f32 padding_) : delta(delta_), padding(padding_) {};
Segment::Segment(Vector2f origin_, Vector2f delta_, f32 padding_) : origin(origin_), delta(delta_), padding(padding_) {};

// https://noonat.github.io/intersect/#intersection-tests
// "time" is the percentage that the segment travels before colliding
bool Segment::isIntersecting(const AABB& aabb) const {
    const Vector2f scale(1.0f / delta.x, 1.0f / delta.y);
    const Vector2f sign(math::sign(delta.x), math::sign(delta.y));

    const Vector2f pos = aabb.getPosition().as<f32>();
    const Vector2f half = aabb.getHalf().as<f32>();
    const Vector2f paddingV = Vector2f(padding, padding);
    const Vector2f nearTime = (pos - sign * (half + paddingV) - origin) * scale;
    const Vector2f farTime = (pos + sign * (half + paddingV) - origin) * scale;

    if (nearTime.x > farTime.y || nearTime.y > farTime.x) {
        return false;
    }

    const f32 maxNearTime = nearTime.x > nearTime.y ? nearTime.x : nearTime.y;
    const f32 minFarTime = farTime.x < farTime.y ? farTime.x : farTime.y;

    if (maxNearTime >= 1.0f || minFarTime <= 0.0f) {
        return false;
    }

    // Collision is happening
    return true;
}

// "time" is the percentage that the segment travels before colliding
RaycastHit Segment::collide(const AABB& aabb) const {
    const Vector2f scale(1.0f / delta.x, 1.0f / delta.y);
    const Vector2f sign(math::sign(delta.x), math::sign(delta.y));

    const Vector2f pos = aabb.getPosition().as<f32>();
    const Vector2f half = aabb.getHalf().as<f32>();
    const Vector2f paddingV = Vector2f(padding, padding);
    const Vector2f nearTime = (pos - sign * (half + paddingV) - origin) * scale;
    const Vector2f farTime = (pos + sign * (half + paddingV) - origin) * scale;

    if (nearTime.x > farTime.y || nearTime.y > farTime.x) {
        return RaycastHit();
    }

    const f32 maxNearTime = nearTime.x > nearTime.y ? nearTime.x : nearTime.y;
    const f32 minFarTime = farTime.x < farTime.y ? farTime.x : farTime.y;

    if (maxNearTime >= 1.0f || minFarTime <= 0.0f) {
        return RaycastHit();
    }

    // Collision is happening
    Vector2i hitnormal;
    if (nearTime.x > nearTime.y) {
        hitnormal.x = -sign.x;
    } else {
        hitnormal.y = -sign.y;
    }

    const f32 time = math::clamp(maxNearTime, 0.0f, 1.0f);
    const Vector2f hitPos = origin + delta * time;
    return RaycastHit(-1, hitPos.round(), (delta * time).len(), hitnormal);  // use dummy entity ID
}

}  // namespace whal
