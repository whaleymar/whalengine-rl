#pragma once

#include "Util/DynamicFactory.h"
#include "json_fwd.hpp"

template <typename T>
struct Vector2;

namespace whal {

namespace ecs {
class Entity;
}

using EntityBuilder = void (*)(ecs::Entity entity, const nlohmann::json& tiledTemplate, ecs::Entity parent, Vector2<float> parentSize);
class EntityFactory : public DynamicFactory<EntityBuilder> {
public:
    EntityFactory();
};

}  // namespace whal
