#pragma once

#include <vector>

#include "Physics/CollisionLayer.h"
#include "Physics/Shapes.h"

namespace whal {

struct Transform2D;

namespace ecs {
class Entity;
}

using TriggerCallback = void (*)(ecs::Entity self, ecs::Entity other);

struct Trigger {
    Trigger() = default;
    Trigger(Shape shape_, CollisionLayer::Layer layer_, TriggerCallback callbackEnter, TriggerCallback callbackExit = nullptr,
            TriggerCallback callbackStay = nullptr);

    Shape shape;
    CollisionLayer::Layer layer;
    TriggerCallback onTriggerEnter;
    TriggerCallback onTriggerExit;
    TriggerCallback onTriggerStay;
    std::vector<ecs::Entity> insideEntities;
};

}  // namespace whal
