#pragma once

#include "Events/Events.h"
#include "Game/Components/Respawn.h"

#include "Systems/Event.h"
#include "whalECS/src/ECS.h"

class RespawnListener : public whal::ecs::ISystem<Respawn> {
public:
    RespawnListener();

private:
    whal::EventListener<whal::ecs::Entity> mDeathListener;
};
