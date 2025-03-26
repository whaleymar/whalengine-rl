#pragma once

#include <vector>

#include "Physics/CollisionLayer.h"
#include "Physics/Shapes.h"

namespace whal {

struct Transform;

namespace ecs {
class Entity;
}

using TriggerCallback = void (*)(ecs::Entity self, ecs::Entity other);

struct Trigger {
    Shape shape;
    Vector2i offset;  // offset from transform
    u16 layerMask = CollisionLayer::ActorPhysics;
    TriggerCallback onTriggerEnter = nullptr;
    TriggerCallback onTriggerExit = nullptr;
    TriggerCallback onTriggerStay = nullptr;
    std::vector<ecs::Entity> insideEntities;
};

}  // namespace whal
