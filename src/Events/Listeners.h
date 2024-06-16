#pragma once

#include "Events.h"

namespace whal {
namespace ecs {
class Entity;
}

// This is registered with the ECS as the callback that runs when an entity dies
void emitEntityDeathEvent(ecs::Entity entity);

}  // namespace whal
