#include "TweenSystem.h"

#include "Components/Collision.h"
#include "Components/Transform.h"
#include "Components/TriggerZone.h"
#include "Components/Tween.h"
#include "Sys/System.h"

namespace whal {

// RESEARCH collider support for this system is... primitive. Doesn't support callbacks and can only be used for Solids (no updates if we get pushed
// by another object... if we can get pushed, entity should have a velocity component)
void TweenPositionSystem::update() {
    const f32 dtModified = System::dt();
    const f32 dtUnmodified = System::time.getUnmodified();
    for (auto [entityid, entity] : getEntitiesCopy()) {
        auto& tweenPos = entity.get<TweenPosition>();
        const f32 dt = tweenPos.isSet(TweenParams::IgnoreSlowdown) ? dtUnmodified : dtModified;
        if (tweenPos.mDelay > tweenPos.mElapsedTime) {
            tweenPos.mElapsedTime += dt;
            continue;
        }
        auto& trans = entity.get<Transform2D>();

        const f32 progress = (tweenPos.mElapsedTime - tweenPos.mDelay) / tweenPos.duration;
        Vector2f nextPosition = ease(tweenPos.mStartPosition, tweenPos.mEndPosition, progress, tweenPos.easing);

        if (entity.has<Collider>()) {
            auto& collider = entity.get<Collider>();
            assert(collider.getCollisionLayer() == CollisionLayer::Solid && "Should only use TweenPosition with Solid Colliders");
            Vector2f toMove;
            if (entity.has<PrecisePosition>()) {
                auto& precisePos = entity.get<PrecisePosition>();
                toMove = nextPosition - precisePos.position;
            } else {
                toMove = nextPosition - trans.position.as<f32>();
            }

            // entity does not have velocity, so we don't need to worry about rigidbody flags
            collider.move(toMove, nullptr, false, true);

            // this system handles fractional movement itself
            collider.mXRemainder = 0.0f;
            collider.mYRemainder = 0.0f;

        } else {
            if (entity.has<PrecisePosition>()) {
                entity.get<PrecisePosition>().position = nextPosition;
            }
            trans.position = nextPosition.round();
        }

        // RESEARCH could maybe have an EntityMoved event that trigger system can listen for? Because triggers are not moved by this...
        if (entity.has<Trigger>()) {
            auto trigger = entity.get<Trigger>();
            Transform2D adjustedTransform = Transform2D(trans.position + trigger.offset);
            trigger.shape.setPosition(adjustedTransform);
            entity.set(trigger);
        }

        if (progress >= 1.0) {
            if (tweenPos.mNumLoops == 0) {
                entity.remove<TweenPosition>();
            } else {
                s32 nLoopsRemaining = tweenPos.mNumLoops == -1 ? -1 : tweenPos.mNumLoops - 1;
                TweenPosition newTween = tweenPos;
                newTween.setLoops(nLoopsRemaining);
                newTween.mElapsedTime = 0.0;
                newTween.resetFlag(TweenParams::CustomOrigin);
                if (tweenPos.isSet(TweenParams::Bounce)) {
                    newTween.target *= -1;
                }
                entity.remove<TweenPosition>();
                entity.add(newTween);
            }
        } else {
            tweenPos.mElapsedTime += dt;
        }
    }
}

// sets start position in struct, and updates target to world coords if it is a relative coord
void TweenPositionSystem::onAdd(ecs::Entity entity) {
    auto& tweenPos = entity.get<TweenPosition>();
    const Vector2i position = tweenPos.isSet(TweenParams::CustomOrigin) ? tweenPos.mStartPosition.as<s32>() : entity.get<Transform2D>().position;

    tweenPos.mStartPosition = position.as<f32>();
    if (tweenPos.isSet(TweenParams::RelativeTarget)) {
        tweenPos.mEndPosition = (position + tweenPos.target).as<f32>();
    } else {
        tweenPos.mEndPosition = tweenPos.target.as<f32>();
    }
}

}  // namespace whal
