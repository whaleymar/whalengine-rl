#pragma once

#include "Util/Vector.h"

namespace whal {

struct HitInfo;
struct RaycastHit;
class AABB;

struct Segment {
    Vector2f origin;
    Vector2f delta;
    f32 padding;

    Segment(Vector2f delta_, f32 padding = 0);
    Segment(Vector2f origin_, Vector2f delta_, f32 padding = 0);

    bool isIntersecting(const AABB& other) const;
    RaycastHit collide(const AABB& other) const;
};

}  // namespace whal
