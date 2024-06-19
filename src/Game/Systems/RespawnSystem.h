#pragma once

#include "Events/Events.h"
#include "Game/Components/Respawn.h"

#include "Systems/System.h"
#include "whalECS/src/ECS.h"

class RespawnListener : public whal::ecs::ISystem<Respawn>, public whal::IListen<whal::DeathEvent, whal::ecs::Entity> {
public:
    void onEvent(whal::ecs::Entity) override;
};
