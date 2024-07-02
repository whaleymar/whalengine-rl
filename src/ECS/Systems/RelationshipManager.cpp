#include "RelationshipManager.h"

#include "Settings.h"

#include "ECS/Relationships.h"
#include "ECS/Transform.h"
#include "ECS/Velocity.h"
#include "Events/Events.h"
#include "Util/MathUtil.h"
#include "Util/Vector.h"

namespace whal {

// removes entity from child list
void EntityChildSystem::onEvent(DeathEvent, ecs::Entity entity) {
    for (auto [entityid, parent] : getEntitiesRef()) {
        auto& children = parent.get<Children>();
        auto it = ecs::whal_find(children.entityIDs.begin(), children.entityIDs.end(), entity.id());
        if (it != children.entityIDs.end()) {
            children.entityIDs.erase(it);
        }
    }
}

void EntityChildSystem::onRemove(ecs::Entity entity) {
    std::vector<ecs::EntityID> childrencopy = std::move(entity.get<Children>().entityIDs);
    for (auto childEntityID : childrencopy) {
        ecs::Entity childEntity(childEntityID);
        childEntity.kill();
    }
}

void AttachSystem::onAdd(ecs::Entity entity) {
    entity.get<Attach>().initTarget(entity);
}

void AttachSystem::fixedUpdate() {
    for (auto [entityid, entity] : getEntitiesRef()) {
        Transform2D& trans = entity.get<Transform2D>();
        Attach attach = entity.get<Attach>();
        ecs::Entity targetEntity(attach.targetEntityID);
        trans.position = targetEntity.get<Transform2D>().position + attach.offsetTexels * PIXELS_PER_TEXEL;
    }
}

void FollowSystem::fixedUpdate() {
    for (auto [entityid, entity] : getEntitiesRef()) {
        Transform2D trans = entity.get<Transform2D>();
        auto& follow = entity.get<Follow>();
        if (!follow.isTargetInitialized) {
            follow.initTarget(entity);
        }

        ecs::Entity targetEntity(follow.targetEntityID);
        Transform2D targetTrans = targetEntity.get<Transform2D>();
        // consider target speed if it has the component and adjust lookahead to be smaller for low speeds
        f32 lookAheadX = follow.lookAheadTexels.x();
        f32 lookAheadY = follow.lookAheadTexels.y();
        bool isTargetMovingX = false;
        bool isTargetMovingY = false;
        bool isMovingUp = false;
        if (auto velOpt = targetEntity.tryGet<Velocity>(); velOpt) {
            f32 velx = (*velOpt)->total.x();
            f32 vely = (*velOpt)->total.y();
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
            ((targetTrans.position.x() <= trans.position.x() && trans.position.x() <= follow.currentTarget.x()) ||
             (targetTrans.position.x() >= trans.position.x() && trans.position.x() >= follow.currentTarget.x()))) {
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
            targetSpeedX = myLerp(vel.stable.x(), targetSpeedX, follow.damping.x());
        }
        if (abs(targetSpeedY) > abs(vel.stable.y())) {
            targetSpeedY = myLerp(vel.stable.y(), targetSpeedY, follow.damping.y());
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
        ecs::Entity debugTargetTracker(follow.debugTargetTrackerID);
        ecs::Entity debugPositionTracker(follow.debugPositionTrackerID);

        debugTargetTracker.set(Transform2D(follow.currentTarget));
        debugPositionTracker.set(trans);
#endif  // !NDEBUG
    }
}

void FollowSystem::onRemove(ecs::Entity entity) {
    // reset any lingering effects on velocity
    if (entity.has<Velocity>()) {
        entity.set(Velocity());
    }
}

// if the target of an entity's Follow component dies, remove the follow component.
void FollowSystem::onEvent(DeathEvent, ecs::Entity killedEntity) {
    std::vector<ecs::Entity> toRemove;
    for (auto& [entityid, entity] : FollowSystem::getEntitiesRef()) {
        if (entity.get<Follow>().targetEntityID == killedEntity.id()) {
            toRemove.push_back(entity);
        }
    }

    for (auto entity : toRemove) {
        entity.remove<Follow>();
    }
}

}  // namespace whal
