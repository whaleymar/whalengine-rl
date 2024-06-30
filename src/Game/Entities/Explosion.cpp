#include "Explosion.h"

#include "ECS/Callback.h"
#include "ECS/Light.h"
#include "ECS/PlayerControl.h"
#include "ECS/Systems/TagTrackers.h"
#include "Game/Components/Blaster.h"
#include "Physics/CollisionLayer.h"
#include "Physics/HitInfo.h"
#include "Physics/Shapes.h"
#include "Settings.h"
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
    constexpr f32 lifetime = 0.5;
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
        const auto& otherCollider = other.get<Collider>().getShape();

        Trigger& trigger = self.get<Trigger>();
        Vector2i center = trigger.shape.getPosition();
        Vector2f delta = toFloatVec(otherCollider.getPosition() - center);
        auto unitDelta = delta.isZero() ? Vector2f::zero : delta.norm();

        // slight knockback falloff based on distance
        auto circle = trigger.shape.getCircle();
        f32 distanceMultiplier = 1 - std::pow(circle.getDistanceFromCenter(&otherCollider) / circle.getRadius(), 2);
        PushStrength cPushStrength = self.get<PushStrength>();

        Velocity& vel = other.get<Velocity>();
        // vel.stable += unitDelta * pushStrengthMax * Vector2f(multX, multY);
        auto impulse = unitDelta * distanceMultiplier * cPushStrength.strength;
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
    entity.add(Sprite(Depth::Foreground1, animator.getFrame()));

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
