#include "Explosion.h"

#include "ECS/Systems/TagTrackers.h"
#include "Game/Components/Blaster.h"
#include "Physics/CollisionLayer.h"
#include "Physics/Shapes.h"
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

Expected<whal::ecs::Entity> makeExplosionZone(Vector2i center, s32 halflen) {
    using namespace whal;

    auto eEntity = System::ecs->entity(false);
    if (!eEntity.isExpected()) {
        return eEntity;
    }

    auto entity = eEntity.value();
    auto _ = ecs::DeferActivate(entity);

    Transform2D trans = Transform2D(center - Vector2i(0, halflen));
    entity.add(trans);

    TriggerCallback pushEntityAway = [](ecs::Entity self, ecs::Entity other) {
        const auto& otherCollider = other.get<Collider>().getShape();

        Trigger& trigger = self.get<Trigger>();
        Vector2i center = trigger.shape.getPosition();
        Vector2f delta = toFloatVec(otherCollider.getPosition() - center);
        auto unitDelta = delta.norm();

        // slight knockback falloff based on distance
        auto circle = trigger.shape.getCircle();
        f32 pushMult = 1 - std::pow(circle.getDistanceFromCenter(&otherCollider) / circle.getRadius(), 2);

        const Vector2f pushStrengthMax = {100, 100};

        Velocity& vel = other.get<Velocity>();
        // vel.stable += unitDelta * pushStrengthMax * Vector2f(multX, multY);
        vel.stable += unitDelta * pushStrengthMax * pushMult;

        // ----------------------------
        // ADD ROCKET JUMPING COMPONENT
        if (!other.has<RocketJumping>()) {
            other.add<RocketJumping>();
        }
    };
    auto shape = Shape(Circle(trans, halflen));
    auto trigger = Trigger(shape, CollisionLayer::TriggerActors, pushEntityAway);
    entity.add(trigger);

    constexpr f32 lifetime = 0.5;
    constexpr s32 nFrames = 6;
    constexpr f32 frameTime = lifetime / static_cast<f32>(nFrames);
    static const AnimInfo animInfo = {{"effect/explosion", 0, nFrames, frameTime}};
    Animator animator;
    loadAnimations(animator, animInfo);
    animator.setLooping(false);
    entity.add(animator);
    entity.add(Sprite(Depth::Foreground1, animator.getFrame()));

    entity.add(Lifetime(lifetime));

    // scale volume with distance from camera
    f32 distance = toFloatVec(getCameraPosition() - trans.position).len();
    f32 maxVolume = 0.2f;
    f32 maxDistance = 1500.0f;
    f32 volume = easeOutQuad(maxVolume, 0.0f, distance / maxDistance);
    System::audio.playClip(Sfx::EXPLOSION, volume, AudioPlayer::Filter::None, false, &trans.position);
    // System::audio.playClip(Sfx::EXPLOSION, volume);

    return entity;
}
