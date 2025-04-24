#pragma once

#include "Material.h"
#include "Shapes.h"

#include "Components/Collider.h"
#include "Components/Draw.h"
#include "Components/Lifetime.h"
#include "Components/Light.h"
#include "Components/RigidBody.h"
#include "Components/Transform.h"
#include "Components/Velocity.h"
#include "Sys/JobScheduler.h"
#include "Sys/System.h"

#include "Util/Vector.h"

namespace rl {
typedef struct Color Color;
}

namespace whal {

namespace ecs {
class Entity;
}

enum class ParticleShape { Rect, Sprite, BezierQuad, Line };

struct MaterialData {
    enum Flags : u32 {
        None = 0,
        Collision = 1,
        RigidBodyFlag = 1 << 1,
        Liquid = 1 << 2,
        Light = 1 << 3,
        DecayTime = 1 << 4,      // lifetime is based on material's DecayTimeParams
        DecaySpeed = 1 << 5,     // lifetime is based on material's DecaySpeedParams
        FadeOutFlag = 1 << 6,    // tweens colors towards `fadeColor`. if `Light` flag is set then tweens the radius towards zero
        ScaleUp = 1 << 7,        // from min to max (Scaling Down is default)
        ScaleBounce = 1 << 8,    // from max to min to max (or min to max to min if ScaleUp is set)
        RandomSpinDir = 1 << 9,  // randomly choose between clockwise/counterclockwise spinning
    };

    struct DecayTimeParams {
        f32 decaySecondsMin = 0.5;
        f32 decaySecondsMax = 2.0;
    };

    struct DecaySpeedParams {
        f32 minSpeedTPS = 0.01;
        f32 decaySeconds = 0.25;
    };

    static const MaterialData& get(WorldMaterial material);
    bool isFlagSet(Flags flag) const { return (flags & flag) > 0; }
    f32 getDecayTime() const;
    Color getColor() const;

    // T is a draw-like component. Must have a member called ".color" and one called "brightness"
    template <typename T>
    void addComponents(ecs::Entity entity, s32 halfLen, Color color, f32 lifetimeMultiplier = 1.0) const {
        const f32 lifetime = getDecayTime() * lifetimeMultiplier;
        entity.add(Lifetime{.secondsRemaining = lifetime});

        if (isFlagSet(Collision)) {
            auto collider = Collider(entity.get<Transform>(), {halfLen, halfLen}, PhysicsBody::Feather, CollisionLayer::ActorPhysics);
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
            entity.add(PointLight{.radius = halfLen * 2, .heightOffset = 0, .color = color});

            // if fading, then decrease light with time
            if (isFlagSet(FadeOutFlag)) {
                Schedule.tween(entity, 0, lifetime, &PointLight::radius).setTransition(Ease::InQuad);
            }
        }

        if (isFlagSet(DecaySpeed)) {
            // want to add this one after some delay, in case particle gains speed in first few frames (like from gravity or something)
            Schedule.flow({entity}).addWait(0.25).add(
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
                    Schedule.tween(e, fadeColor, decaySeconds, &T::color)
                        .setTransition(Ease::InOutQuad)
                        .setOnStart([](ecs::Entity self, const Tween<Color>&) {
                            self.add(Lifetime{.secondsRemaining = self.get<DieWhenSpeedBelow>().lifetime});
                        })
                        .setDelayCondition([](ecs::Entity self, const Tween<Color>&) -> bool {
                            return self.has<Velocity>() && self.get<Velocity>().total.len() >= self.get<DieWhenSpeedBelow>().minSpeed;
                        })
                        .setOnEnd([](ecs::Entity self, const Tween<Color>&) { self.remove<DieWhenSpeedBelow>(); });
                },
                entity, decayParams.decaySpeed.minSpeedTPS, decayParams.decaySpeed.decaySeconds, color, fadeColor);
        }

        if (isFlagSet(FadeOutFlag)) {
            Schedule.tween(entity, fadeColor, lifetime, &T::color).setTransition(Ease::InQuad);
        }

        if (brightness != 1.0) {
            entity.get<T>().color.scale(brightness);
        }

        if (minScale != maxScale) {
            Tweener<Vector2f> tween;
            f32 tweenTime = isFlagSet(ScaleBounce) ? lifetime * 0.5f : lifetime;
            if (isFlagSet(ScaleUp)) {
                tween = Schedule.tween(entity, Vector2f(maxScale, maxScale), tweenTime, &Transform::scale, &Transform::setScale)
                            .from(Vector2f(minScale, minScale));
            } else {
                // by default, scale down
                tween = Schedule.tween(entity, Vector2f(minScale, minScale), tweenTime, &Transform::scale, &Transform::setScale)
                            .from(Vector2f(maxScale, maxScale));
            }

            if (isFlagSet(ScaleBounce)) {
                tween.asBounce();
            }
        } else if (minScale != 1.0f) {
            // minScale and maxScale are the same, but not 1, so set the scale normally
            entity.get<Transform>().setScale(Vector2f(minScale, minScale), entity);
        }

        if (maxRotationsPerSec != 0.0f || minRotationsPerSec != 0.0f) {
            f32 modifier = 1.0f;
            if (isFlagSet(RandomSpinDir) && Rng.uniform() > 0.5f) {
                modifier = -1.0f;
            }
            entity.add(AngularVelocity{.rotationsPerSecond = Rng.range(minRotationsPerSec, maxRotationsPerSec) * modifier});
        }
    }

    const char* name;
    WorldMaterial id;
    Color colorRange[2];
    u32 flags = DecayTime | FadeOutFlag;
    f32 bounciness = 0.0;
    f32 gravityCoef = 1.0;
    Vector2f frictionCoefs = {1.0, 1.0};
    Color fadeColor = Colors::ClearWhite;
    union {
        DecayTimeParams decayTime;
        DecaySpeedParams decaySpeed;
    } decayParams;
    f32 minScale = 1.0f;
    f32 maxScale = 1.0f;
    ParticleShape particleShape = ParticleShape::Rect;
    f32 brightness = 1.0;
    f32 minRotationsPerSec = 0.0f;
    f32 maxRotationsPerSec = 0.0f;
};

}  // namespace whal
