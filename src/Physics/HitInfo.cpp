#include "Physics/HitInfo.h"
#include "Util/Vector.h"

namespace whal {

HitInfo::HitInfo() {}

HitInfo::HitInfo(Vector2i normal) {
    flags |= CollisionInfo::Hit;

    if (normal.x() > 0) {
        flags |= CollisionInfo::Right;
    } else if (normal.x() < 0) {
        flags |= CollisionInfo::Left;
    }

    if (normal.y() > 0) {
        flags |= CollisionInfo::Above;
    } else if (normal.y() < 0) {
        flags |= CollisionInfo::Below;
    }
}

Vector2i HitInfo::toVec() const {
    Vector2i normal;
    if (flags & CollisionInfo::Right) {
        normal.e[0] = 1;
    } else if (flags & CollisionInfo::Left) {
        normal.e[0] = -1;
    }

    if (flags & CollisionInfo::Above) {
        normal.e[1] = 1;
    } else if (flags & CollisionInfo::Below) {
        normal.e[1] = -1;
    }

    return normal;
}

}  // namespace whal
