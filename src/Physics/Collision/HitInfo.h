#pragma once

#include "Physics/Material.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

// enum CollisionInfo : u8 {
//     Hit = 0,
//     Above = 1,
//     Below = 1 << 1,
//     Left = 1 << 2,
//     Right = 1 << 3,
//     Grounded = 1 << 4,
//     HeadBonk = 1 << 5,
//     Solid = 1<<6,
//     SemiSolid
// };

// once i hit 17 bytes I'll use the bitfield
struct HitInfo {
    Vector2i normal;
    ecs::Entity other;
    bool isOtherSolid = false;
    bool isOtherSemiSolid = false;
    bool isOtherActor = false;
    WorldMaterial otherMaterial = WorldMaterial::None;

    HitInfo();
    HitInfo(Vector2i normal);
};

}  // namespace whal
