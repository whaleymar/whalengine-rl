#pragma once

#include "Material.h"
#include "Shapes.h"

#include "Components/Collision.h"
#include "Components/Draw.h"
#include "Components/Lifetime.h"
#include "Components/Light.h"
#include "Components/RigidBody.h"
#include "Components/Velocity.h"
#include "Sys/System.h"

#include "Util/Vector.h"

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
        GlowFlag = 1 << 4,
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

    // T is a draw-like component. Must have a member called ".color" and one called "brightness"
    template <typename T>
    void addComponents(ecs::Entity entity, s32 halfLen, Color color, f32 lifetimeMultiplier = 1.0) const {
        const f32 lifetime = getDecayTime() * lifetimeMultiplier;
        entity.add(Lifetime(lifetime));

        if (isFlagSet(Collision)) {
            auto collider = Collider::Actor(AABB(entity.get<Transform2D>(), {halfLen, halfLen}, Vector2i()));
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
            entity.add(PointLight{halfLen * 2, 0, color});

            // if fading, then decrease light with time
            if (isFlagSet(FadeOutFlag)) {
                Schedule.tween(entity, 0, lifetime, [](ecs::Entity self) -> s32& { return self.get<PointLight>().radius; })
                    .setTransition(Ease::InQuad);
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
                    Schedule.tween(e, fadeColor, decaySeconds, [](ecs::Entity self) -> Color& { return self.get<T>().color; })
                        .setTransition(Ease::InOutQuad)
                        .setOnStart([](ecs::Entity self, const Tween<Color>&) { self.add(Lifetime(self.get<DieWhenSpeedBelow>().lifetime)); })
                        .setDelayCondition([](ecs::Entity self, const Tween<Color>&) -> bool {
                            return self.has<Velocity>() && self.get<Velocity>().total.len() >= self.get<DieWhenSpeedBelow>().minSpeed;
                        })
                        .setOnEnd([](ecs::Entity self, const Tween<Color>&) { self.remove<DieWhenSpeedBelow>(); });
                },
                entity, decayParams.decaySpeed.minSpeedTPS, decayParams.decaySpeed.decaySeconds, color, fadeColor);
        }

        if (isFlagSet(FadeOutFlag)) {
            Schedule.tween(entity, fadeColor, lifetime, [](ecs::Entity self) -> Color& { return self.get<T>().color; })
                .setTransition(Ease::InOutQuad);
        }

        if (isFlagSet(GlowFlag)) {
            entity.get<T>().brightness = 2.0;  // TODO brightness modifier in material struct, not flag
        }

        if (startScale != 1.0) {
            Schedule.tween(entity, Vector2f(1.0, 1.0), lifetime / 2, [](ecs::Entity self) -> Vector2f& { return self.get<Transform2D>().scale; })
                .from(Vector2f(1.0, 1.0) * startScale);
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
