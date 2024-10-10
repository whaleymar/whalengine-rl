#include "EntityFactory.h"

#include "Components/Callback.h"
#include "Components/Collision.h"
#include "Components/Draw.h"
#include "Components/RailsControl.h"
#include "Components/TriggerZone.h"
#include "whalECS/src/ECS.h"

namespace whal {

static void addDeathTriggerCallback(ecs::Entity entity, const nlohmann::json& tiledTemplate, const ActiveLevel& activeLevel);
void addDeathCollisionCallback(ecs::Entity entity, const nlohmann::json& tiledTemplate, const ActiveLevel& activeLevel);

static const NameToCreator<EntityBuilder> S_ENTITY_ENTRIES[] = {
    {"DeathTriggerBase", addDeathTriggerCallback},
    {"DeathCollisionCallback", addDeathCollisionCallback},
};

EntityFactory::EntityFactory() : DynamicFactory<EntityBuilder>("EntityFactory", S_ENTITY_ENTRIES) {}

void addDeathTriggerCallback(ecs::Entity entity, const nlohmann::json& tiledTemplate, const ActiveLevel& activeLevel) {
    if (!entity.has<Trigger>()) {
        entity.add<Trigger>();
    }
    entity.get<Trigger>().onTriggerEnter = [](ecs::Entity self, ecs::Entity other) { other.kill(); };
}

void addDeathCollisionCallback(ecs::Entity entity, const nlohmann::json& tiledTemplate, const ActiveLevel& activeLevel) {
    if (!entity.has<Collider>()) {
        entity.add<Collider>();
    }
    entity.get<Collider>().setCollisionCallback([](ecs::Entity self, ecs::Entity other, Vector2i hitNormal) { other.kill(); });
}

}  // namespace whal
