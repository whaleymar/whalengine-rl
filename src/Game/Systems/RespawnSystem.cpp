#include "RespawnSystem.h"

#include "ECS/Draw.h"
#include "Systems/System.h"

void RespawnListener::onEvent(whal::ecs::Entity entity) {
    if (!RespawnListener::getEntitiesRef().contains(entity.id())) {
        return;
    }

    auto respawn = entity.get<Respawn>();
    whal::Sprite sprite;  // needs to be created in main thread bc OpenGL

    // clang-format off
    whal::System::schedule.eventFlow()
        .add(respawn.onDeath)
        .addWait(respawn.waitTime)
        .add(respawn.respawnCallback, sprite)
        .add(respawn.onRespawn);
    // clang-format on
}
