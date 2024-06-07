#pragma once

#include <vector>

#include "Physics/Collision/ICollider.h"
// #include "Physics/Collision/Shapes.h"

namespace whal {

struct Transform2D;

namespace ecs {
class Entity;
}

using TriggerCallback = void (*)(ecs::Entity self, ecs::Entity other);

struct Trigger {
    Trigger() = default;
    Trigger(Shape shape_, TriggerCallback callbackEnter, TriggerCallback callbackExit = nullptr);

    Shape shape;
    TriggerCallback onTriggerEnter;
    TriggerCallback onTriggerExit;
    std::vector<ecs::Entity> insideEntities;
};

}  // namespace whal
