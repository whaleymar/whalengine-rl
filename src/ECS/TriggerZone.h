#pragma once

#include <vector>

#include "Physics/Collision/ICollider.h"
#include "Physics/Collision/Shapes.h"

namespace whal {

struct Transform2D;

namespace ecs {
class Entity;
}

using TriggerCallback = void (*)(ecs::Entity self, ecs::Entity other);

// struct Trigger {
//     Trigger() = default;
//     Trigger(IColliderShape* shape, TriggerCallback callbackEnter, TriggerCallback callbackExit = nullptr);
//
//     std::unique_ptr<IColliderShape> shape;
//     TriggerCallback onTriggerEnter;
//     TriggerCallback onTriggerExit;
//     std::vector<ecs::Entity> insideEntities;
// };

// a bounding box which executes callbacks on the actors it holds
struct TriggerZone : public AABB2 {
    TriggerZone() = default;
    TriggerZone(Transform2D transform, Vector2i halflen, TriggerCallback callbackEnter, TriggerCallback callbackExit = nullptr);

    TriggerCallback onTriggerEnter;
    TriggerCallback onTriggerExit;
    std::vector<ecs::Entity> insideEntities;
};

// TODO should be combined with ^ ?
// TODO onTriggerStay
struct TriggerCircle : public Circle2 {
    TriggerCircle() = default;
    TriggerCircle(Transform2D transform, s32 radius, TriggerCallback callbackEnter, TriggerCallback callbackExit = nullptr);

    TriggerCallback onTriggerEnter;
    TriggerCallback onTriggerExit;
    std::vector<ecs::Entity> insideEntities;
};

}  // namespace whal
