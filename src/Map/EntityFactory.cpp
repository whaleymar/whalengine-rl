#include "EntityFactory.h"

#include "whalECS/src/ECS.h"
#include "ECS/TriggerZone.h"
#include "Game/Entities/Checkpoint.h"

namespace whal {

static NameToCreator<EntityBuilder> S_ENTITY_ENTRIES[] = {
    {"SpawnPointTrigger", createRespawnTriggerPrefab},
};

EntityFactory::EntityFactory() : Factory<EntityBuilder>("EntityFactory") {
    initFactory(S_ENTITY_ENTRIES);
}

void createRespawnTriggerPrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel) {
    entity.get<Trigger>().onTriggerEnter = &onCheckpointEnter;
}

}  // namespace whal
