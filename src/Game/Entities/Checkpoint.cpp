#include "Checkpoint.h"

#include "ECS/Transform.h"
#include "Game/Components/Respawn.h"

void onCheckpointEnter(whal::ecs::Entity self, whal::ecs::Entity other) {
    if (other.has<Respawn>() && other.has<IUseCheckpoints>()) {
        auto& respawn = other.get<Respawn>();
        respawn.spawnPosition = self.get<whal::Transform2D>().position;
    }
}
