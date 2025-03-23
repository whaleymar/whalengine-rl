#pragma once

#include <vector>

#include "Map/ComponentFactory.h"
#include "Physics/CollisionLayer.h"
#include "Physics/Shapes.h"

namespace whal {

struct Transform;

namespace ecs {
class Entity;
}

using TriggerCallback = void (*)(ecs::Entity self, ecs::Entity other);

struct Trigger : ISerialize<Trigger, ComponentFactory> {
    Shape shape;
    Vector2i offset;  // offset from transform
    u16 layerMask = CollisionLayer::ActorPhysics;
    TriggerCallback onTriggerEnter = nullptr;
    TriggerCallback onTriggerExit = nullptr;
    TriggerCallback onTriggerStay = nullptr;
    std::vector<ecs::Entity> insideEntities;

    static void loadImpl(ecs::Entity entity, const LoadContext& ctx);
};

}  // namespace whal
