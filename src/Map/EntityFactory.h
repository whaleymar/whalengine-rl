#pragma once

#include "Util/DynamicFactory.h"
#include "json_fwd.hpp"

namespace whal {

namespace ecs {
class Entity;
}

using EntityBuilder = void (*)(ecs::Entity entity, const nlohmann::json& tiledTemplate, ecs::Entity parent);
class EntityFactory : public DynamicFactory<EntityBuilder> {
public:
    EntityFactory();
};

}  // namespace whal
