#pragma once

#include "Util/Factory.h"
#include "json_fwd.hpp"

namespace whal {

namespace ecs {
class Entity;
}

using EntityBuilder = ecs::Entity (*)(const nlohmann::json& tiledTemplate, ActiveLevel&);
class EntityFactory : public Factory<EntityBuilder> {
public:
    EntityFactory();
};

}  // namespace whal
