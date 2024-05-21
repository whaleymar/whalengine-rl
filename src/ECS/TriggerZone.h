#pragma once

#include "Physics/Collision/AABB.h"

namespace whal {

struct Transform;

namespace ecs {
class Entity;
}

using TriggerCallback = void (*)(ecs::Entity self, ecs::Entity other);

// a bounding box which executes callbacks on the actors it holds
struct TriggerZone : public AABB {
    TriggerZone() = default;
    TriggerZone(Transform transform, Vector2i halflen, TriggerCallback callbackEnter, TriggerCallback callbackExit = nullptr);

    TriggerCallback onTriggerEnter;
    TriggerCallback onTriggerExit;
    std::vector<ecs::Entity> insideEntities;
};

}  // namespace whal
