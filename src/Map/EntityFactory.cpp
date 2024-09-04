#include "EntityFactory.h"

#include "Components/Callback.h"
#include "Components/Collision.h"
#include "Components/Draw.h"
#include "Components/Light.h"
#include "Components/RailsControl.h"
#include "Components/RigidBody.h"
#include "Components/Tags.h"
#include "Components/TriggerZone.h"
#include "Components/Velocity.h"
#include "Game/Components/Blaster.h"
#include "Game/Components/ProjectileInfo.h"
#include "Game/Components/Switch.h"
#include "Game/Entities/Checkpoint.h"
#include "Game/Entities/Explosion.h"
#include "Systems/TagTrackers.h"
#include "whalECS/src/ECS.h"

namespace whal {

static void createRespawnTriggerPrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, const ActiveLevel& activeLevel);
static void addDeathTriggerCallback(ecs::Entity entity, const nlohmann::json& tiledTemplate, const ActiveLevel& activeLevel);
void addDeathCollisionCallback(ecs::Entity entity, const nlohmann::json& tiledTemplate, const ActiveLevel& activeLevel);
// void addSpikeTileCC(ecs::Entity entity, const nlohmann::json& tiledTemplate, const ActiveLevel& activeLevel);
static void createSwitch(ecs::Entity entity, const nlohmann::json& tiledTemplate, const ActiveLevel& activeLevel);
static void createAppearTrigger(ecs::Entity entity, const nlohmann::json& tiledTemplate, const ActiveLevel& activeLevel);
static void createJumpThruTrigger(ecs::Entity entity, const nlohmann::json& tiledTemplate, const ActiveLevel& activeLevel);

static NameToCreator<EntityBuilder> S_ENTITY_ENTRIES[] = {
    {"SimpleRespawnTrigger", createRespawnTriggerPrefab},
    {"DeathTriggerBase", addDeathTriggerCallback},
    {"DeathCollisionCallback", addDeathCollisionCallback},
    {"MultiSwitch", createSwitch},
    {"AppearTrigger", createAppearTrigger},
    {"JumpThruTrigger", createJumpThruTrigger},
};

EntityFactory::EntityFactory() : Factory<EntityBuilder>("EntityFactory") {
    initFactory(S_ENTITY_ENTRIES);
}

// static void addBobbingTween(ecs::Entity entity) {
//     TweenManager::add(TweenInt(2, 0.8 + System::rng.range(0.0f, 0.4f), [](ecs::Entity self) -> int& { return self.get<Transform2D>().position.y; })
//                           .asRelative()
//                           .setLoops(-1)
//                           .asBounce()
//                           .setDelay(System::rng.range(0.0f, 0.5f))
//                           .setTransition(Ease::InOutQuad),
//                       entity);
// }

void createRespawnTriggerPrefab(ecs::Entity entity, const nlohmann::json& tiledTemplate, const ActiveLevel& activeLevel) {
    entity.get<Trigger>().onTriggerEnter = &onCheckpointEnter;
}

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

// void addSpikeTileCC(ecs::Entity entity, const nlohmann::json& tiledTemplate, const ActiveLevel& activeLevel) {
//     if (!entity.has<Collider>()) {
//         entity.add<Collider>();
//     }
//     auto& collider = entity.get<Collider>();
//     collider.setCollisionCallback([](ecs::Entity self, ecs::Entity other, Vector2i hitNormal) { other.kill(); });
//
//     const auto shape = collider.getShape();
//     switch (collider.getCollisionDir()) {
//     case CollisionDir::UP:
//         collider.setShape(AABB(shape.getPosition() - Vector2i(0, 2), {4, 2}));
//         break;
//     case CollisionDir::DOWN:
//         collider.setShape(AABB(shape.getPosition() + Vector2i(0, 4), {4, 2}));
//         break;
//     case CollisionDir::LEFT:
//         collider.setShape(AABB(shape.getPosition() + Vector2i(2, 0), {2, 4}));
//         break;
//     case CollisionDir::RIGHT:
//         collider.setShape(AABB(shape.getPosition() - Vector2i(2, 0), {2, 4}));
//         break;
//     default:
//         break;
//     }
// }

void createSwitch(ecs::Entity entity, const nlohmann::json& tiledTemplate, const ActiveLevel& activeLevel) {
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

void createAppearTrigger(ecs::Entity entity, const nlohmann::json& tiledTemplate, const ActiveLevel& activeLevel) {
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

void createJumpThruTrigger(ecs::Entity entity, const nlohmann::json& tiledTemplate, const ActiveLevel& activeLevel) {
    const auto callback = [](ecs::Entity self, ecs::Entity other) {
        if (!(other.has<Player>() && other.has<Velocity>() && other.has<RigidBody>())) {
            return;
        }

        constexpr f32 minVelocity = 60.0f;
        auto& velocity = other.get<Velocity>();
        if (velocity.total.y > 0.0f && velocity.stable.y < minVelocity) {
            velocity.stable.y = minVelocity;
        }
    };

    auto& trigger = entity.get<Trigger>();
    trigger.onTriggerEnter = callback;
    trigger.onTriggerStay = callback;
}

}  // namespace whal
