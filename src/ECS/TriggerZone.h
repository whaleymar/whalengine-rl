#pragma once

#include "Physics/Collision/AABB.h"
#include "Physics/Collision/CircleCollider.h"

namespace whal {

struct Transform2D;

namespace ecs {
class Entity;
}

using TriggerCallback = void (*)(ecs::Entity self, ecs::Entity other);

// a bounding box which executes callbacks on the actors it holds
struct TriggerZone : public AABB {
    TriggerZone() = default;
    TriggerZone(Transform2D transform, Vector2i halflen, TriggerCallback callbackEnter, TriggerCallback callbackExit = nullptr);

    TriggerCallback onTriggerEnter;
    TriggerCallback onTriggerExit;
    std::vector<ecs::Entity> insideEntities;
};

// TODO should be combined with ^ ?
// TODO onTriggerStay
struct TriggerCircle : public Circle {
    TriggerCircle() = default;
    TriggerCircle(Transform2D transform, s32 radius, TriggerCallback callbackEnter, TriggerCallback callbackExit = nullptr);

    TriggerCallback onTriggerEnter;
    TriggerCallback onTriggerExit;
    std::vector<ecs::Entity> insideEntities;
};

}  // namespace whal
