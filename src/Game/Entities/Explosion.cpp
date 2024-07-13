#include "Explosion.h"

#include "ECS/Callback.h"
#include "ECS/Light.h"
#include "ECS/PlayerControl.h"
#include "ECS/Systems/TagTrackers.h"
#include "Game/Components/Blaster.h"
#include "Game/Components/ProjectileInfo.h"
#include "Game/MathUtil.h"
#include "Physics/CollisionLayer.h"
#include "Physics/HitInfo.h"
#include "Physics/Shapes.h"
#include "Settings.h"
#include "Util/Print.h"
#include "whalECS/src/ECS.h"

#include "Gfx/Depth.h"
#include "Systems/System.h"
#include "Util/MathUtil.h"

#include "ECS/AnimUtil.h"
#include "ECS/Animator.h"
#include "ECS/Collision.h"
#include "ECS/Draw.h"
#include "ECS/Lifetime.h"
#include "ECS/Transform.h"
#include "ECS/TriggerZone.h"
#include "ECS/Velocity.h"

constexpr f32 RJ_DECAY = 0.95;

struct PushStrength {
    Vector2f strength;
};

Expected<whal::ecs::Entity> makeExplosionZone(Vector2i center, s32 halflen, Vector2f pushStrength) {
    using namespace whal;

    auto eEntity = System::world->entity(false);
    if (!eEntity.isExpected()) {
        return eEntity;
    }

    auto entity = eEntity.value();
    auto _ = ecs::DeferActivate(entity);

    // ANIMATOR
    constexpr f32 lifetime = 0.25;
    constexpr s32 nFrames = 6;
    constexpr f32 frameTime = lifetime / static_cast<f32>(nFrames);
    static const AnimInfo animInfo = {{"effect/explosion", 0, nFrames, frameTime}};
    Animator animator;
    loadAnimations(animator, animInfo);
    animator.setLooping(false);
    // ----

    Transform2D trans = Transform2D(center - Vector2i(0, animator.getFrame().dimensionsTexels.y() * PIXELS_PER_TEXEL / 2));
    entity.add(trans);

    entity.add(PushStrength(pushStrength));

    TriggerCallback pushEntityAway = [](ecs::Entity self, ecs::Entity other) {
        if (other.has<ProjectileInfo>()) {
            return;
        }
        auto& otherCollider = other.get<Collider>();
        const auto& otherShape = otherCollider.getShape();

        Trigger& trigger = self.get<Trigger>();
        Vector2i center = trigger.shape.getPosition();
        Vector2f delta = toFloatVec(otherShape.getPosition() - center);
        Vector2f unitDelta = delta.isZero() ? Vector2f::zero : closestOrdinalDirection(delta.norm());

        // slight knockback falloff based on distance
        auto circle = trigger.shape.getCircle();
        f32 distanceFromCenter = circle.getDistanceFromCenter(&otherShape);

        // ok, what if instead of all this junk, i do uniform distance multiplier, and just set the other entity's transform to be on the trigger's
        // boundary in whatever direction they're being pushed?
        // const Vector2f moveVec = unitDelta * (static_cast<f32>(circle.getRadius()) - distanceFromCenter);
        // otherCollider.move(moveVec, nullptr, false, true);

        // f32 distanceMultiplier = 1 - std::pow(distanceFromCenter / static_cast<f32>(circle.getRadius()), 3);
        // f32 distanceMultiplier = 1 - distanceFromCenter / static_cast<f32>(circle.getRadius());
        f32 distanceMultiplier = 1;

        PushStrength cPushStrength = self.get<PushStrength>();

        Velocity& vel = other.get<Velocity>();
        // vel.stable += unitDelta * pushStrengthMax * Vector2f(multX, multY);
        const bool isRJStateOn = other.has<RocketJumping>();
        auto impulse = unitDelta * distanceMultiplier * cPushStrength.strength;

        // changing direction, so zero X velocity
        // for each axis we're boosting in, check if we're changing direction
        // if we are, zero current velocity in that direction before applying force
        // otherwise, check if we're already in an RJ state. If we are, reduce the additional force we're adding
        if (impulse.x() != 0) {
            if (sign(vel.stable.x()) != sign(impulse.x())) {
                vel.stable.e[0] = 0;

            } else if (isRJStateOn) {
                impulse.e[0] *= RJ_DECAY;
            }
        }

        if (impulse.y() != 0) {
            if (sign(vel.stable.y()) != sign(impulse.y())) {
                vel.stable.e[1] = 0;

            } else if (isRJStateOn) {
                impulse.e[1] *= RJ_DECAY;
            }
        }

        // for debugging stability:
        if (other.has<Player>()) {
            print("circle center: ", circle.getPosition());
            print("player center: ", otherShape.getPosition());
            print("Distance from explosion center: ", distanceFromCenter);
            // print("(rounded from): ", prevValue);
            print("Distance Multiplier: ", distanceMultiplier);
            print("Player Velocity before push: ", vel.stable);
            // print("Player Velocity.impulse before push: ", vel.impulse);
            // print("Player Velocity.residualimpulse before push: ", vel.residualImpulse);
            print("unitDelta: ", unitDelta);
            print("strength: ", cPushStrength.strength);
            print("Impulse force: ", impulse);
            print("");
        }
        vel.stable += impulse;

        // ----------------------------
        // ADD ROCKET JUMPING COMPONENT
        if (!other.has<RocketJumping>() && other.has<PlayerControl>()) {
            other.add<RocketJumping>();
        }
    };
    auto shape = Shape(Circle(center, halflen));
    auto trigger = Trigger(shape, CollisionLayer::TriggerActors, pushEntityAway);
    entity.add(trigger);

    entity.add(animator);
    entity.add(Draw(Sprite(Depth::Foreground1, animator.getFrame())));

    entity.add(Lifetime(lifetime));
    entity.add(PointLight({TEXELS_PER_TILE * 5, halflen / PIXELS_PER_TEXEL}));

    // scale volume with distance from camera
    f32 distance = toFloatVec(getCameraPosition() - trans.position).len();
    f32 maxVolume = 0.2f;
    f32 maxDistance = 1500.0f;
    f32 volume = easeOutQuad(maxVolume, 0.0f, distance / maxDistance);
    System::audio.playClip(Sfx::EXPLOSION, volume, AudioPlayer::Filter::None, false, &trans.position);
    // System::audio.playClip(Sfx::EXPLOSION, volume);

    return entity;
}
