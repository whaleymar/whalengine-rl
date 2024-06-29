#include "EntityFactory.h"
#include "Util/Print.h"

namespace whal {

static NameToCreator<EntityBuilder> S_ENTITY_ENTRIES[] = {
    {"TestPrefab", createTestPrefab},
};

EntityFactory::EntityFactory() : Factory<EntityBuilder>("EntityFactory") {
    initFactory(S_ENTITY_ENTRIES);
}

void createTestPrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel) {
    print("HERE in createTestPrefab");
}

}  // namespace whal
