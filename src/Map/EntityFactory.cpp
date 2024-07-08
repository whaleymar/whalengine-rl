#include "EntityFactory.h"

#include "ECS/Callback.h"
#include "ECS/Collision.h"
#include "ECS/Draw.h"
#include "ECS/RailsControl.h"
#include "ECS/RigidBody.h"
#include "ECS/TriggerZone.h"
#include "ECS/Velocity.h"
#include "Game/Components/Switch.h"
#include "Game/Entities/Checkpoint.h"
#include "whalECS/src/ECS.h"

namespace whal {

static void createRespawnTriggerPrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);
static void createWeightedPlatformPrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);
static void createDeathZonePrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);
static void createRubbleFallSwitch(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);
static void createRailsMoveSwitch(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);

static NameToCreator<EntityBuilder> S_ENTITY_ENTRIES[] = {
    {"SpawnPointTrigger", createRespawnTriggerPrefab}, {"WeightedPlatform", createWeightedPlatformPrefab}, {"DeathTrigger", createDeathZonePrefab},
    {"RubbleFallSwitch", createRubbleFallSwitch},      {"RailsMoveSwitch", createRailsMoveSwitch},
};

EntityFactory::EntityFactory() : Factory<EntityBuilder>("EntityFactory") {
    initFactory(S_ENTITY_ENTRIES);
}

void createRespawnTriggerPrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel) {
    entity.get<Trigger>().onTriggerEnter = &onCheckpointEnter;
}

void createWeightedPlatformPrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel) {
    auto collisionCallback = [](ecs::Entity callbackEntity, ecs::Entity other, Collider* callbackEntityCollider, Collider* otherCollider,
                                Vector2i hitNormal) {
        auto& rails = callbackEntity.get<RailsControl>();
        if (rails.isWaiting && rails.curTarget == 0) {
            rails.startManually();
            callbackEntity.get<Draw>().setColor(GREEN);
        }
    };

    auto onMoveDone = [](ecs::Entity entity, RailsControl& railsControl) { entity.get<Draw>().setColor(RED); };

    entity.get<Collider>().setCollisionCallback(collisionCallback);
    entity.get<RailsControl>().arrivalCallback = onMoveDone;
}

void createDeathZonePrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel) {
    entity.get<Trigger>().onTriggerEnter = [](ecs::Entity self, ecs::Entity other) { other.kill(); };
}

void createRubbleFallSwitch(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel) {
    entity.get<Trigger>().onTriggerEnter = [](ecs::Entity self, ecs::Entity other) {
        auto flipSwitch = self.get<Switch>();
        ecs::EntityID targetID = flipSwitch.target;

        ecs::Entity target(targetID);
        target.add<RigidBody>();
        target.add<Velocity>();
        if (target.has<Draw>()) {
            target.get<Draw>().setColor(GREEN);
        }

        if (self.has<Draw>()) {
            self.get<Draw>().setColor(GREEN);
        }
        self.add(OnFrameEnd([](ecs::Entity e) { e.remove<Trigger>(); }));
    };
}

void createRailsMoveSwitch(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel) {
    entity.get<Trigger>().onTriggerEnter = [](ecs::Entity self, ecs::Entity other) {
        auto flipSwitch = self.get<Switch>();
        ecs::EntityID targetID = flipSwitch.target;

        ecs::Entity target(targetID);
        auto& rails = target.get<RailsControl>();
        rails.startManually();
        rails.arrivalCallback = [](ecs::Entity e, RailsControl& rails) { e.add(OnFrameEnd([](ecs::Entity e) { e.remove<RailsControl>(); })); };
        if (target.has<Draw>()) {
            target.get<Draw>().setColor(GREEN);
        }

        if (self.has<Draw>()) {
            self.get<Draw>().setColor(GREEN);
        }
        self.add(OnFrameEnd([](ecs::Entity e) { e.remove<Trigger>(); }));
    };
}

}  // namespace whal
