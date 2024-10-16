#pragma once

#include "Components/Light.h"
#include "Components/RigidBody.h"
#include "Components/Velocity.h"
#include "Material.h"

#include "Components/Draw.h"
#include "Util/Vector.h"

#include "Components/Collision.h"
#include "Components/Lifetime.h"
#include "Components/Tween.h"

#include "Physics/Shapes.h"

namespace whal {

namespace ecs {
class Entity;
}

struct MaterialData {
    enum Flags : u8 {
        None = 0,
        Collision = 1,
        RigidBodyFlag = 1 << 1,
        Liquid = 1 << 2,
        Light = 1 << 3,
        RadianceFlag = 1 << 4,
        DecayTime = 1 << 5,
        DecaySpeed = 1 << 6,
        FadeOutFlag = 1 << 7,
    };

    struct DecayTimeParams {
        f32 decaySecondsMin = 0.5;
        f32 decaySecondsMax = 2.0;
    };

    struct DecaySpeedParams {
        f32 minSpeedTPS = 0.01;
        f32 decaySeconds = 0.25;
    };

    static MaterialData get(WorldMaterial material);
    bool isFlagSet(Flags flag) const { return (flags & flag) > 0; }
    f32 getDecayTime() const;
    Color getColor() const;

    // TODO `requires std::isbaseof<IDraw, T>` and use common interface
    // T is the draw component
    template <typename T>
    void addComponents(ecs::Entity entity, s32 halfLen, Color color, f32 lifetimeMultiplier = 1.0) const {
        const f32 lifetime = getDecayTime() * lifetimeMultiplier;
        entity.add(Lifetime(lifetime));

        if (isFlagSet(Collision)) {
            auto collider = Collider::Actor(AABB(entity.get<Transform2D>(), {halfLen, halfLen}));
            collider.setMaterial(id);
            entity.add(collider);
        }

        if (isFlagSet(RigidBodyFlag)) {
            auto rigidBody = RigidBody();
            rigidBody.gravityMultiplier = gravityCoef;
            rigidBody.frictionMultiplier = frictionCoefs;
            entity.add(rigidBody);
        }

        if (isFlagSet(Light)) {
            entity.add(PointLight{halfLen * 2});
        }

        if (isFlagSet(DecaySpeed)) {
            // want to add this one after some delay, in case particle gains speed in first few frames (like from gravity or something)
            System::schedule.eventFlow({entity}).addWait(0.5).add(
                [](ecs::Entity e, f32 minSpeedTPS, f32 decaySeconds, Color color, Color fadeColor) {
                    // Can't capture data in a lambda? Use a component! ECS!!! :D
                    struct DieWhenSpeedBelow {
                        f32 minSpeed;
                        f32 lifetime;
                    };
                    e.add(DieWhenSpeedBelow{
                        .minSpeed = minSpeedTPS,
                        .lifetime = decaySeconds,
                    });
                    TweenManager::add(
                        TweenColor(fadeColor, decaySeconds, [](ecs::Entity self) -> Color& { return self.get<T>().color; })
                            .setTransition(Ease::InOutQuad)
                            .setOnStart([](ecs::Entity self, const TweenColor&) { self.add(Lifetime(self.get<DieWhenSpeedBelow>().lifetime)); })
                            .setDelayCondition([](ecs::Entity self, const TweenColor&) -> bool {
                                return !self.has<Velocity>() || self.get<Velocity>().total.len() <= self.get<DieWhenSpeedBelow>().minSpeed;
                            })
                            .setOnEnd([](ecs::Entity self, const TweenColor&) { self.remove<DieWhenSpeedBelow>(); }),
                        e);
                },
                entity, decayParams.decaySpeed.minSpeedTPS, decayParams.decaySpeed.decaySeconds, color, fadeColor);
        }

        if (isFlagSet(FadeOutFlag)) {
            TweenManager::add(
                TweenColor(fadeColor, lifetime, [](ecs::Entity self) -> Color& { return self.get<T>().color; }).setTransition(Ease::InOutQuad),
                entity);
        }

        if (isFlagSet(RadianceFlag)) {
            entity.add(Radiance{2, 0, color});
        }

        if (startScale != 1.0) {
            TweenManager::add(TweenVec2f(Vector2f(1.0, 1.0), lifetime / 2, [](ecs::Entity self) -> Vector2f& { return self.get<T>().scale; })
                                  .from(Vector2f(1.0, 1.0) * startScale),
                              entity);
        }
    }

    const char* name;
    WorldMaterial id;
    Color colorRange[2];
    u8 flags = DecayTime | FadeOutFlag;
    f32 bounciness = 0.0;
    f32 gravityCoef = 1.0;
    Vector2f frictionCoefs = {1.0, 1.0};
    Color fadeColor = Color(255, 255, 255, 0);
    union {
        DecayTimeParams decayTime;
        DecaySpeedParams decaySpeed;
    } decayParams;
    f32 startScale = 1.0;
    DrawTag particleShape = DrawTag::Rect;
};

}  // namespace whal
