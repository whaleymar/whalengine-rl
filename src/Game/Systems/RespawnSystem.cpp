#include "RespawnSystem.h"

#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Game/Components/Respawn.h"
#include "Sys/System.h"

void RespawnListener::onEvent(whal::DeathEvent, whal::ecs::Entity entity) {
    if (!RespawnListener::getEntitiesMutable().contains(entity.id())) {
        return;
    }

    auto respawn = entity.get<Respawn>();
    whal::Transform2D transform(respawn.spawnPosition);

    // clang-format off
    whal::System::schedule.eventFlow()
        .add(respawn.onDeath)
        .addWait(respawn.waitTime)
        .add(respawn.spawnFunction, transform)
        .add(respawn.onRespawn);
    // clang-format on
}
