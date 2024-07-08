#pragma once

#include "Util/Factory.h"
#include "json_fwd.hpp"

namespace whal {

namespace ecs {
class Entity;
}

struct ActiveLevel;

using EntityBuilder = void (*)(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);
class EntityFactory : public Factory<EntityBuilder> {
public:
    EntityFactory();
};

}  // namespace whal
