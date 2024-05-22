#include "RelationshipManager.h"

#include "Settings.h"

#include "ECS/Relationships.h"
#include "ECS/Transform.h"
#include "ECS/Velocity.h"
#include "Game/Events.h"
#include "Systems/System.h"
#include "Util/MathUtil.h"
#include "Util/Print.h"
#include "Util/Vector.h"

namespace whal {

EntityChildSystem::EntityChildSystem() : mEntityDeathListener(&removeEntityFromChildList) {
    System::eventMgr.registerListener(Event::DEATH_EVENT, mEntityDeathListener);
}

void EntityChildSystem::onRemove(ecs::Entity entity) {
    std::vector<ecs::Entity> childrencopy = std::move(entity.get<Children>().entities);
    for (auto childEntity : childrencopy) {
        childEntity.kill();
    }
}

FollowSystem::FollowSystem() : mEntityDeathListener(&unfollowEntity) {
    System::eventMgr.registerListener(Event::DEATH_EVENT, mEntityDeathListener);
}

void FollowSystem::update() {
    for (auto [entityid, entity] : getEntitiesRef()) {
        Transform2D trans = entity.get<Transform2D>();
        auto& follow = entity.get<Follow>();
        if (!follow.isTargetInitialized) {
            follow.initTarget(entity);
        }
        Transform2D targetTrans = follow.targetEntity.get<Transform2D>();
        // consider target speed if it has the component and adjust lookahead to be smaller for low speeds
        f32 lookAheadX = follow.lookAheadTexels.x();
        f32 lookAheadY = follow.lookAheadTexels.y();
        bool isTargetMovingX = false;
        bool isTargetMovingY = false;
        bool isMovingUp = false;
        if (auto velOpt = follow.targetEntity.tryGet<Velocity>(); velOpt) {
            f32 velx = velOpt.value()->total.x();
            f32 vely = velOpt.value()->total.y();
            lookAheadX *= clamp(abs(velx) * 0.1f, 0.0f, 1.0f);
            lookAheadY *= clamp(abs(vely) * 0.1f, 0.0f, 1.0f);
            isTargetMovingX = abs(velx) > 1;
            isTargetMovingY = vely != 0;
            isMovingUp = vely > 0;
        }

        s32 target;
        if (follow.isMovingX && isTargetMovingX) {
            s32 direction = targetTrans.facing == Facing::Right ? 1 : -1;
            target = targetTrans.position.x() + direction * lookAheadX * PIXELS_PER_TEXEL;
        } else {
            target = targetTrans.position.x();
        }
        s32 min = follow.boundsXTexels.e[0] * PIXELS_PER_TEXEL;
        s32 max = follow.boundsXTexels.e[1] * PIXELS_PER_TEXEL;
        s32 currentTarget = clamp(target, min, max);
        s32 distanceFromTarget = abs(currentTarget - trans.position.x()) * PIXELS_PER_TEXEL;

        // if the target is not moving, we shouldn't move away from it
        if (follow.isMovingX && !isTargetMovingX &&
            ((targetTrans.position.x() <= trans.position.x() <= follow.currentTarget.x()) ||
             (targetTrans.position.x() >= trans.position.x() >= follow.currentTarget.x()))) {
            follow.currentTarget.e[0] = trans.position.x();
            follow.isMovingX = false;
        } else if (distanceFromTarget > follow.deadZoneTexels.x()) {
            follow.currentTarget.e[0] = currentTarget;
            follow.isMovingX = true;
        }

        if (follow.isMovingY && isTargetMovingY) {
            s32 direction = isMovingUp ? 1 : -1;
            target = targetTrans.position.y() + direction * lookAheadY * PIXELS_PER_TEXEL;
        } else {
            target = targetTrans.position.y();
        }
        min = follow.boundsYTexels.e[0] * PIXELS_PER_TEXEL;
        max = follow.boundsYTexels.e[1] * PIXELS_PER_TEXEL;
        currentTarget = clamp(target, min, max);
        distanceFromTarget = abs(currentTarget - trans.position.y()) * PIXELS_PER_TEXEL;

        if (distanceFromTarget > follow.deadZoneTexels.y()) {
            follow.currentTarget.e[1] = currentTarget;
            follow.isMovingY = true;
        }

        // TEMP
        Velocity& vel = entity.get<Velocity>();
        // Velocity vel = entity.get<Velocity>();
        f32 targetSpeedX = static_cast<f32>((follow.currentTarget.x() - trans.position.x())) * FTEXELS_PER_PIXEL;
        f32 targetSpeedY = static_cast<f32>(follow.currentTarget.y() - trans.position.y()) * FTEXELS_PER_PIXEL;

        if (abs(targetSpeedX) > abs(vel.stable.x())) {
            targetSpeedX = lerp(vel.stable.x(), targetSpeedX, follow.damping.x());
        }
        if (abs(targetSpeedY) > abs(vel.stable.y())) {
            targetSpeedY = lerp(vel.stable.y(), targetSpeedY, follow.damping.y());
        }

        vel.stable = {targetSpeedX, targetSpeedY};
        vel.stable *= {2, 2};

        // don't go too slow
        f32 minspeed = 1.91;  // min speed for rounding to not zero at 60fps
        if (vel.stable.x() > 0 && vel.stable.x() < minspeed) {
            vel.stable.e[0] = minspeed;
        } else if (vel.stable.x() < 0 && vel.stable.x() > -minspeed) {
            vel.stable.e[0] = -minspeed;
        }

        if (vel.stable.x() == 0 || !isTargetMovingX) {
            follow.isMovingX = false;
        }

        if (vel.stable.y() > 0 && vel.stable.y() < minspeed) {
            vel.stable.e[1] = minspeed;
        } else if (vel.stable.y() < 0 && vel.stable.y() > -minspeed) {
            vel.stable.e[1] = -minspeed;
        } else if (vel.stable.y() == 0) {
            follow.isMovingY = false;
        }

#ifndef NDEBUG
        // follow.debugTargetTracker.set(Transform(follow.currentTarget));
        // follow.debugPositionTracker.set(trans);
#endif  // !NDEBUG
    }
}

void FollowSystem::onRemove(ecs::Entity entity) {
    // reset any lingering effects on velocity
    if (entity.has<Velocity>()) {
        entity.set(Velocity());
    }
}

void unfollowEntity(ecs::Entity killedEntity) {
    print("RUNNING UNFOLLOW CALLBACK");
    std::vector<ecs::Entity> toRemove;
    for (auto& [entityid, entity] : FollowSystem::getEntitiesRef()) {
        if (entity.get<Follow>().targetEntity == killedEntity) {
            toRemove.push_back(entity);
        }
    }

    for (auto entity : toRemove) {
        entity.remove<Follow>();
    }
}

void removeEntityFromChildList(ecs::Entity entity) {
    for (auto [entityid, parent] : EntityChildSystem::instance()->getEntitiesRef()) {
        auto& children = parent.get<Children>();
        auto it = std::find(children.entities.begin(), children.entities.end(), entity);
        if (it != children.entities.end()) {
            children.entities.erase(it);
        }
    }
}

}  // namespace whal
