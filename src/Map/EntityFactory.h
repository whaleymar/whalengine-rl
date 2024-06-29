#pragma once

#include "Util/Factory.h"
#include "json_fwd.hpp"

namespace whal {

namespace ecs {
class Entity;
}

using EntityBuilder = void (*)(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);
class EntityFactory : public Factory<EntityBuilder> {
public:
    EntityFactory();
};

void createTestPrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);

}  // namespace whal
