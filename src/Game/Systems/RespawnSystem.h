#pragma once

#include "Events/Events.h"

#include "Systems/System.h"
#include "whalECS/src/ECS.h"

struct Respawn;

class RespawnListener : public whal::ecs::ISystem<Respawn>, public whal::IListen<whal::DeathEvent, true, whal::ecs::Entity> {
public:
    void onEvent(whal::ecs::Entity) override;
};
