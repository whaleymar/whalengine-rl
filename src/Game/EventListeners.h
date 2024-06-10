#pragma once

#include "Events.h"

namespace whal::ecs {
class Entity;
}

void emitEntityDeathEvent(whal::ecs::Entity entity);

void onEntityDeath(whal::ecs::Entity entity);

void startListeners();
void killListeners();

namespace Listeners {

inline auto ENTITY_DEATH_LISTENER = whal::EventListener<whal::ecs::Entity>(&onEntityDeath);

}
