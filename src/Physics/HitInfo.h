#pragma once

#include "Physics/CollisionLayer.h"
#include "Physics/Material.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

namespace CollisionInfo {

enum Flags : u8 {
    Hit = 1,
    Above = 1 << 1,
    Below = 1 << 2,
    Left = 1 << 3,
    Right = 1 << 4,
    Collision = 1 << 5,
    Push = 1 << 6,
    Carry = 1 << 7,
};

constexpr u8 VERTICAL = Above | Below;
constexpr u8 HORIZONTAL = Right | Left;

}  // namespace CollisionInfo

struct HitInfo {
    ecs::Entity other;
    u8 flags = 0;
    WorldMaterial otherMaterial = WorldMaterial::None;
    CollisionLayer::Layer otherLayer = CollisionLayer::None;  // so i know if it was a solid, semisolid, etc. w/out fetching component

    HitInfo();
    HitInfo(Vector2i normal, bool isCollision = false, bool isPush = false, bool isCarry = false);

    operator bool() const { return flags & CollisionInfo::Hit; }
    Vector2i toVec() const;
    bool isUp() const { return flags & CollisionInfo::Above; }
    bool isDown() const { return flags & CollisionInfo::Below; }
    bool isRight() const { return flags & CollisionInfo::Right; }
    bool isLeft() const { return flags & CollisionInfo::Left; }
    bool isVertical() const { return flags & (CollisionInfo::VERTICAL); }
    bool isHorizontal() const { return flags & (CollisionInfo::HORIZONTAL); }
};

}  // namespace whal
