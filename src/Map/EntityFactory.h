#pragma once

#include "Util/DynamicFactory.h"
#include "json_fwd.hpp"

namespace whal {

namespace ecs {
class Entity;
}

struct ActiveLevel;

using EntityBuilder = void (*)(ecs::Entity entity, const nlohmann::json& tiledTemplate, const ActiveLevel& activeLevel);
class EntityFactory : public DynamicFactory<EntityBuilder> {
public:
    EntityFactory();
};

}  // namespace whal
