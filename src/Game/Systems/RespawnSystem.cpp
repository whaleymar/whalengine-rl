#include "RespawnSystem.h"

#include "ECS/Draw.h"
#include "ECS/Transform.h"
#include "Game/Components/Respawn.h"
#include "Systems/System.h"

void RespawnListener::onEvent(whal::ecs::Entity entity) {
    if (!RespawnListener::getEntitiesRef().contains(entity.id())) {
        return;
    }

    auto respawn = entity.get<Respawn>();
    whal::Sprite sprite;  // needs to be created in main thread bc OpenGL
    whal::Transform2D transform(respawn.spawnPosition);

    // clang-format off
    whal::System::schedule.eventFlow()
        .add(respawn.onDeath)
        .addWait(respawn.waitTime)
        .add(respawn.spawnFunction, transform, sprite)
        .add(respawn.onRespawn);
    // clang-format on
}
