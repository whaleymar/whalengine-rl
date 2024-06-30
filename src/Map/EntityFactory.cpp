#include "EntityFactory.h"
#include "ECS/TriggerZone.h"
#include "Game/Entities/Checkpoint.h"
#include "Util/Print.h"

namespace whal {

static NameToCreator<EntityBuilder> S_ENTITY_ENTRIES[] = {
    {"TestPrefab", createTestPrefab},
    {"SpawnPointTrigger", createRespawnTriggerPrefab},
};

EntityFactory::EntityFactory() : Factory<EntityBuilder>("EntityFactory") {
    initFactory(S_ENTITY_ENTRIES);
}

void createTestPrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel) {
    print("HERE in createTestPrefab");
}

void createRespawnTriggerPrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel) {
    print("HERE in createRespawnTriggerPrefab");

    entity.get<Trigger>().onTriggerEnter = &onCheckpointEnter;
}

}  // namespace whal
