#pragma once

#include "Util/DynamicFactory.h"
class JsonValue;

namespace whal {

namespace ecs {
class Entity;
}

using EntityBuilder = void (*)(ecs::Entity entity, JsonValue tiledTemplate, ecs::Entity parent);
class EntityFactory : public DynamicFactory<EntityBuilder> {
public:
    EntityFactory();
};

}  // namespace whal
