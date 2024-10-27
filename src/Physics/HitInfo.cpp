#include "Physics/HitInfo.h"

#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

HitInfo::HitInfo(Vector2i normal, bool isCollision, bool isPush, bool isCarry) {
    flags |= CollisionInfo::Hit;
    if (isCollision) {
        flags |= CollisionInfo::Collision;
    } else if (isPush) {
        flags |= CollisionInfo::Push;
    } else if (isCarry) {
        flags |= CollisionInfo::Carry;
    }

    if (normal.x > 0) {
        flags |= CollisionInfo::Right;
    } else if (normal.x < 0) {
        flags |= CollisionInfo::Left;
    }

    if (normal.y > 0) {
        flags |= CollisionInfo::Above;
    } else if (normal.y < 0) {
        flags |= CollisionInfo::Below;
    }
}

Vector2i HitInfo::toVec() const {
    Vector2i normal;
    if (flags & CollisionInfo::Right) {
        normal.x = 1;
    } else if (flags & CollisionInfo::Left) {
        normal.x = -1;
    }

    if (flags & CollisionInfo::Above) {
        normal.y = 1;
    } else if (flags & CollisionInfo::Below) {
        normal.y = -1;
    }

    return normal;
}

ecs::Entity HitInfo::getOther() const {
    return ecs::Entity(otherID);
}

void HitInfo::setOther(ecs::Entity e) {
    otherID = e.id();
}

RaycastHit::RaycastHit(ecs::Entity other, Vector2i point_, f32 distance_, Vector2i normal) {
    flags |= RayCollisionInfo::Hit;
    otherID = other.id();
    point = point_;
    distance = distance_;

    if (normal.x > 0) {
        flags |= RayCollisionInfo::Right;
    } else if (normal.x < 0) {
        flags |= RayCollisionInfo::Left;
    }

    if (normal.y > 0) {
        flags |= RayCollisionInfo::Above;
    } else if (normal.y < 0) {
        flags |= RayCollisionInfo::Below;
    }
}

Vector2i RaycastHit::toVec() const {
    Vector2i normal;
    if (flags & RayCollisionInfo::Right) {
        normal.x = 1;
    } else if (flags & RayCollisionInfo::Left) {
        normal.x = -1;
    }

    if (flags & RayCollisionInfo::Above) {
        normal.y = 1;
    } else if (flags & RayCollisionInfo::Below) {
        normal.y = -1;
    }

    return normal;
}

ecs::Entity RaycastHit::getOther() const {
    return ecs::Entity(otherID);
}

void RaycastHit::setOther(ecs::Entity e) {
    otherID = e.id();
}

}  // namespace whal
