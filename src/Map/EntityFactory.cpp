#include "EntityFactory.h"

#include "ECS/Callback.h"
#include "ECS/Collision.h"
#include "ECS/Draw.h"
#include "ECS/Light.h"
#include "ECS/RailsControl.h"
#include "ECS/RigidBody.h"
#include "ECS/Tags.h"
#include "ECS/TriggerZone.h"
#include "ECS/Velocity.h"
#include "Game/Components/Blaster.h"
#include "Game/Components/Switch.h"
#include "Game/Entities/Checkpoint.h"
#include "Game/Save/EventFlags.h"
#include "whalECS/src/ECS.h"

namespace whal {

static void createRespawnTriggerPrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);
static void createWeightedPlatformPrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);
static void createDeathZonePrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);
static void createRubbleFallSwitch(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);
static void createMagicHat(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);
static void createSwitch(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);

static NameToCreator<EntityBuilder> S_ENTITY_ENTRIES[] = {
    {"SpawnPointTrigger", createRespawnTriggerPrefab},
    {"WeightedPlatform", createWeightedPlatformPrefab},
    {"DeathTrigger", createDeathZonePrefab},
    {"RubbleFallSwitch", createRubbleFallSwitch},
    {"Magic Hat", createMagicHat},
    {"MultiSwitch", createSwitch},
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
        if (other.has<Particle>() || hitNormal.y() != 1) {
            return;
        }
        auto& rails = callbackEntity.get<RailsControl>();
        if (rails.isWaiting && rails.curTarget == 0) {
            rails.startManually();
            callbackEntity.get<Draw>().setColor(Colors::LightBlue);
        }
    };

    auto onMoveDone = [](ecs::Entity entity, RailsControl& railsControl) { entity.get<Draw>().setColor(Colors::Pink); };

    entity.get<Collider>().setCollisionCallback(collisionCallback);
    entity.get<RailsControl>().arrivalCallback = onMoveDone;
}

void createDeathZonePrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel) {
    entity.get<Trigger>().onTriggerEnter = [](ecs::Entity self, ecs::Entity other) { other.kill(); };
}

void createRubbleFallSwitch(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel) {
    entity.get<Collider>().setCollisionCallback(
        [](ecs::Entity self, ecs::Entity other, Collider* callbackEntityCollider, Collider* otherCollider, Vector2i hitNormal) {
            auto flipSwitch = self.get<Switch>();
            ecs::EntityID targetID = flipSwitch.target;

            ecs::Entity target(targetID);
            target.add<RigidBody>();
            target.add<Velocity>();
            if (target.has<Draw>()) {
                target.get<Draw>().setColor(Colors::LightBlue);
            }

            if (self.has<Draw>()) {
                self.get<Draw>().setColor(Colors::LightBlue);
            }
            self.add(OnFrameEnd([](ecs::Entity e) { e.remove<Trigger>(); }));
        });
}

void createMagicHat(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel) {
    entity.get<Trigger>().onTriggerEnter = [](ecs::Entity self, ecs::Entity other) {
        if (!other.has<Player>()) {
            return;
        }

        auto flipSwitch = self.get<Switch>();
        ecs::EntityID targetID = flipSwitch.target;

        ecs::Entity target(targetID);

        // TODO eventually want to await until player "Ok"s some dialogue box OR a cutscene ends
        System::schedule.eventFlow({target}).addWait(0.5).add(
            [](ecs::Entity e) {
                auto& rails = e.get<RailsControl>();
                rails.startManually();
                rails.arrivalCallback = [](ecs::Entity e, RailsControl& rails) {
                    e.add(OnFrameEnd([](ecs::Entity e) { e.remove<RailsControl>(); }));
                };
                if (e.has<Draw>()) {
                    e.get<Draw>().setColor(Colors::LightBlue);
                }
            },
            target);

        other.add<Blaster>();
        EventFlags::set(EventFlags::HasMagicHat);

        self.kill();
    };
}

void createSwitch(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel) {
    entity.get<Collider>().setCollisionCallback(
        [](ecs::Entity self, ecs::Entity other, Collider* callbackEntityCollider, Collider* otherCollider, Vector2i hitNormal) {
            auto flipSwitch = self.get<Switch>();
            ecs::EntityID targetID = flipSwitch.target;

            ecs::Entity target(targetID);
            auto& gate = target.get<SwitchGate>();
            gate.numKeys--;
            if (gate.numKeys == 0) {
                auto& rails = target.get<RailsControl>();
                rails.startManually();
                if (target.has<Draw>()) {
                    target.get<Draw>().setColor(Colors::LightBlue);
                }
                // TODO onEnd, if at last checkpoint and persistent, remove rails component
                // then create system with RailsControl and SwitchGate. Listen for player death, and reset to start position if it happens
            }

            if (self.has<Draw>()) {
                self.get<Draw>().setColor(Colors::LightBlue);
            }
            if (self.has<Radiance>()) {
                self.get<Radiance>().color = WHITE;
            }

            self.get<Collider>().setCollisionCallback(nullptr);
        });
}

}  // namespace whal
