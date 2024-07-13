#include "EntityFactory.h"

#include "ECS/AnimUtil.h"
#include "ECS/Animator.h"
#include "ECS/Callback.h"
#include "ECS/Collision.h"
#include "ECS/Draw.h"
#include "ECS/Light.h"
#include "ECS/RailsControl.h"
#include "ECS/RigidBody.h"
#include "ECS/Tags.h"
#include "ECS/Transform.h"
#include "ECS/TriggerZone.h"
#include "ECS/Velocity.h"
#include "Game/Components/Blaster.h"
#include "Game/Components/ProjectileInfo.h"
#include "Game/Components/Switch.h"
#include "Game/Entities/Checkpoint.h"
#include "Game/Entities/Explosion.h"
#include "Game/Save/EventFlags.h"
#include "Settings.h"
#include "whalECS/src/ECS.h"

namespace whal {

static void createRespawnTriggerPrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);
static void createWeightedPlatformPrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);
static void createDeathZonePrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);
static void createRubbleFallSwitch(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);
static void createMagicHat(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);
static void createSwitch(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);
static void createAppearTrigger(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);
static void createBlastCrystal(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel);

static NameToCreator<EntityBuilder> S_ENTITY_ENTRIES[] = {
    {"SpawnPointTrigger", createRespawnTriggerPrefab},
    {"WeightedPlatform", createWeightedPlatformPrefab},
    {"DeathTrigger", createDeathZonePrefab},
    {"RubbleFallSwitch", createRubbleFallSwitch},
    {"Magic Hat", createMagicHat},
    {"MultiSwitch", createSwitch},
    {"AppearTrigger", createAppearTrigger},
    {"BlastCrystal", createBlastCrystal},
};

EntityFactory::EntityFactory() : Factory<EntityBuilder>("EntityFactory") {
    initFactory(S_ENTITY_ENTRIES);
}

void createRespawnTriggerPrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel) {
    entity.get<Trigger>().onTriggerEnter = &onCheckpointEnter;
}

void createWeightedPlatformPrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel) {
    auto collisionCallback = [](ecs::Entity callbackEntity, ecs::Entity other, Vector2i hitNormal) {
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
    entity.get<Collider>().setCollisionCallback([](ecs::Entity self, ecs::Entity other, Vector2i hitNormal) {
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
        System::audio.playClip(Sfx::SWITCH_FLIP, 0.25);
        self.add(OnFrameEnd([](ecs::Entity e) { e.remove<Trigger>(); }));
    });
}

void createMagicHat(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel) {
    const AnimInfo animInfo = {{"actor/magichat", 0, 8, 0.2}};
    Animator animator;
    loadAnimations(animator, animInfo);
    animator.brain = &basicAnimation;
    entity.add(animator);
    entity.add(Draw(Sprite(Depth::Level, animator.getFrame())));
    // entity.add(Sprite(Depth::Level, animator.getFrame(), WHITE, Shaders::Bloom));

    entity.get<Trigger>().onTriggerEnter = [](ecs::Entity self, ecs::Entity other) {
        if (!other.has<Player>()) {
            return;
        }

        auto flipSwitch = self.get<Switch>();
        ecs::EntityID targetID = flipSwitch.target;

        ecs::Entity target(targetID);

        // TODO eventually want to await until player "Ok"s some dialogue box OR a cutscene ends
        // lock controls during this eventflow
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
        System::audio.playClip(Sfx::MAJOR_ITEM_GET, 0.2);

        self.kill();
    };
}

void createSwitch(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel) {
    entity.get<Collider>().setCollisionCallback([](ecs::Entity self, ecs::Entity other, Vector2i hitNormal) {
        if (!other.has<ProjectileInfo>()) {
            return;
        }

        System::audio.playClip(Sfx::SWITCH_FLIP, 0.25);

        auto flipSwitch = self.get<Switch>();
        ecs::EntityID targetID = flipSwitch.target;

        ecs::Entity target(targetID);
        auto& gate = target.get<SwitchGate>();
        if (gate.numKeys > 0) {
            gate.numKeys--;
        }
        if (gate.numKeys == 0) {
            auto& rails = target.get<RailsControl>();
            rails.startManually();
            System::audio.playClip(Sfx::DOOR_OPEN);
            if (target.has<Draw>()) {
                target.get<Draw>().setColor(Colors::LightBlue);
            }
            if (gate.isPersistent) {
                rails.arrivalCallback = [](ecs::Entity self, RailsControl& rails) {
                    if (rails.isAtLastCheckpoint()) {
                        self.add(OnFrameEnd([](ecs::Entity self) -> void { self.remove<RailsControl>(); }));
                    }
                };
            } else if (rails.endBehavior == RailsControl::CycleBehavior::MANUAL_FIRSTSTEP_LOOP ||
                       rails.endBehavior == RailsControl::CycleBehavior::MANUAL_FIRSTSTEP_BACKTRACK) {
                // unset switch once cycle is done
                System::schedule.eventFlow({self}).addWait(1.0f).add(
                    [](ecs::Entity self) {
                        if (self.has<Draw>()) {
                            self.get<Draw>().setColor(Colors::Pink);
                        }

                        if (self.has<Radiance>()) {
                            self.get<Radiance>().color = Color(34, 103, 103, 255);
                        }

                        auto target = ecs::Entity(self.get<Switch>().target);
                        if (target.has<Draw>()) {
                            target.get<Draw>().setColor(Colors::Pink);
                        }
                    },
                    self);
            }
            // TODO create system with RailsControl and SwitchGate (not persistent). Listen for player death, and reset to start position if it
            // happens
        }

        if (self.has<Draw>()) {
            self.get<Draw>().setColor(Colors::LightBlue);
        }
        if (self.has<Radiance>()) {
            self.get<Radiance>().color = WHITE;
        }

        if (gate.isPersistent) {
            // do this at the end of the frame in case multiple colliders trigger this on the same frame
            System::schedule.eventFlow({self}).add([](ecs::Entity self) { self.get<Collider>().setCollisionCallback(nullptr); }, self);
        }
    });
}

void createAppearTrigger(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel) {
    entity.get<Trigger>().onTriggerEnter = [](ecs::Entity self, ecs::Entity other) {
        if (!other.has<Player>()) {
            return;
        }
        ecs::Entity target(self.get<Switch>().target);
        if (target.has<Invisible>()) {
            target.remove<Invisible>();
        }
    };
}

static void createBlastCrystal(ecs::Entity entity, const nlohmann::json& tiledTemplate, ActiveLevel& activeLevel) {
    auto& trigger = entity.get<Trigger>();
    trigger.shape = Shape(Circle(entity.get<Transform2D>(), PIXELS_PER_TILE / 2));
    trigger.onTriggerEnter = [](ecs::Entity self, ecs::Entity other) {
        if (!other.has<Player>()) {
            return;
        }

        const Vector2f explosionStrength(150, 150);
        makeExplosionZone(self.get<Trigger>().shape.getPosition(), PIXELS_PER_TILE, explosionStrength);
    };
}

}  // namespace whal
