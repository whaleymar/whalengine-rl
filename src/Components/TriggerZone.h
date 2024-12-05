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
    Trigger() = default;
    Trigger(Shape shape_, CollisionLayer::Layer layer_, TriggerCallback callbackEnter, Vector2i offset = {0, 0},
            TriggerCallback callbackExit = nullptr, TriggerCallback callbackStay = nullptr);

    Shape shape;
    Vector2i offset;  // offset from transform
    CollisionLayer::Layer layer = CollisionLayer::TriggerActors;
    TriggerCallback onTriggerEnter = nullptr;
    TriggerCallback onTriggerExit = nullptr;
    TriggerCallback onTriggerStay = nullptr;
    std::vector<ecs::Entity> insideEntities;

    static void loadImpl(ecs::Entity entity, void* data);
};

}  // namespace whal
