#include "RelationshipSystems.h"

#include "Components/Name.h"

#include "Components/Relationships.h"
#include "Components/Transform.h"
#include "Components/Velocity.h"
#include "Events/Events.h"
#include "Util/MathUtil.h"
#include "Util/Print.h"
#include "Util/Vector.h"

namespace whal {

// removes entity from child list
void EntityChildSystem::onEvent(evt::Death, ecs::Entity entity) {
    for (auto [entityid, parent] : getEntities()) {
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
        if (childEntity.has<Name>()) {
            print("Killing child entity: ", childEntity.get<Name>(), " -- ID == ", childEntity.id());
        }
        childEntity.kill();
    }
}

void AttachSystem::onAdd(ecs::Entity entity) {
    entity.get<Attach>().initTarget(entity);
}

void AttachSystem::update() {
    for (auto [entityid, entity] : getEntities()) {
        Transform& trans = entity.get<Transform>();
        Attach& attach = entity.get<Attach>();
        const ecs::Entity targetEntity(attach.targetEntityID);
        const auto targetTrans = targetEntity.get<Transform>();
        const Vector2i offsetModifier = (attach.directionParam == Attach::DirectionParam::UseFacingForAll ||
                                         attach.directionParam == Attach::DirectionParam::UseFacingForOffset) &&
                                                targetTrans.facing == Facing::Left ?
                                            Vector2i(-1, 1) :
                                            Vector2i(1, 1);
        const Vector2i targetPosition = targetTrans.apply(attach.offset * offsetModifier);
        if (targetPosition == trans.position) {
            continue;
        }

        if (attach.directionParam == Attach::DirectionParam::UseFacingForAll) {
            trans.position = targetPosition;
            trans.facing = targetTrans.facing;
        } else {
            trans.position = targetPosition;
        }
    }
}

// RESEARCH use collider.move if the entity has a collider? Seems like it would be glitchy if a collision does happen
void OrbitSystem::update() {
    const f32 dt = Time.dt();
    for (auto [entityid, entity] : getEntities()) {
        auto& trans = entity.get<Transform>();
        Orbit& orbit = entity.get<Orbit>();
        const ecs::Entity targetEntity(orbit.targetID);
        if (!orbit.isTargetInitialized) {
            orbit.initTarget(entity);
        }
        const Vector2i orbitTarget = targetEntity.get<Transform>().position + orbit.targetOffset;

        // if we get the current angle and add to that, it has this cool "follow if target moving, orbit if target is still" effect, but not sure if
        // that's useful for anything
        // const auto delta = trans.position - orbitTarget;
        // f32 angle = delta.isZero() ? 0.0f : getAngle(delta.as<f32>());

        // multiply by -1 so rotations are clockwise by default
        const f32 toAdd = -1.0f * 360.0f * orbit.rotationsPerSecond * dt;
        orbit.currentAngle += toAdd;

        // RESEARCH bool param so that entity rotates in sync with orbit? (tidal lock)
        const Vector2f unit = Vector2f::fromAngle(orbit.currentAngle);
        trans.position = (unit * static_cast<f32>(orbit.radius)).round() + orbitTarget + (orbit.selfOffset.as<f32>() * unit).round();
    }
}

void FollowSystem::update() {
    for (auto [entityid, entity] : getEntities()) {
        Transform trans = entity.get<Transform>();
        auto& follow = entity.get<Follow>();
        if (!follow.isTargetInitialized) {
            follow.initTarget(entity);
        }

        ecs::Entity targetEntity(follow.targetEntityID);
        Transform targetTrans = targetEntity.get<Transform>();
        // consider target speed if it has the component and adjust lookahead to be smaller for low speeds
        f32 lookAheadX = follow.lookAhead.x;
        f32 lookAheadY = follow.lookAhead.y;
        bool isTargetMovingX = false;
        bool isTargetMovingY = false;
        bool isMovingUp = false;
        if (auto velOpt = targetEntity.tryGet<Velocity>(); velOpt) {
            f32 velx = velOpt->total.x;
            f32 vely = velOpt->total.y;
            lookAheadX *= math::clamp(math::abs(velx) * 0.1f, 0.0f, 1.0f);
            lookAheadY *= math::clamp(math::abs(vely) * 0.1f, 0.0f, 1.0f);
            isTargetMovingX = math::abs(velx) > 1;
            isTargetMovingY = vely != 0;
            isMovingUp = vely > 0;
        }

        s32 target;
        if (follow.isMovingX && isTargetMovingX) {
            s32 direction = targetTrans.facing == Facing::Right ? 1 : -1;
            target = targetTrans.position.x + direction * lookAheadX;
        } else {
            target = targetTrans.position.x;
        }
        s32 min = follow.boundsX.x;
        s32 max = follow.boundsX.y;
        s32 currentTarget = math::clamp(target, min, max);
        s32 distanceFromTarget = math::abs(currentTarget - trans.position.x);

        // if the target is not moving, we shouldn't move away from it
        if (follow.isMovingX && !isTargetMovingX &&
            ((targetTrans.position.x <= trans.position.x && trans.position.x <= follow.currentTarget.x) ||
             (targetTrans.position.x >= trans.position.x && trans.position.x >= follow.currentTarget.x))) {
            follow.currentTarget.x = trans.position.x;
            follow.isMovingX = false;
        } else if (distanceFromTarget > follow.deadZone.x) {
            follow.currentTarget.x = currentTarget;
            follow.isMovingX = true;
        }

        if (follow.isMovingY && isTargetMovingY) {
            s32 direction = isMovingUp ? 1 : -1;
            target = targetTrans.position.y + direction * lookAheadY;
        } else {
            target = targetTrans.position.y;
        }
        min = follow.boundsY.x;
        max = follow.boundsY.y;
        currentTarget = math::clamp(target, min, max);
        distanceFromTarget = math::abs(currentTarget - trans.position.y);

        if (distanceFromTarget > follow.deadZone.y) {
            follow.currentTarget.y = currentTarget;
            follow.isMovingY = true;
        }

        // TEMP
        Velocity& vel = entity.get<Velocity>();
        // Velocity vel = entity.get<Velocity>();
        f32 targetSpeedX = static_cast<f32>((follow.currentTarget.x - trans.position.x));
        f32 targetSpeedY = static_cast<f32>(follow.currentTarget.y - trans.position.y);

        if (math::abs(targetSpeedX) > math::abs(vel.stable.x)) {
            targetSpeedX = math::lerp(vel.stable.x, targetSpeedX, follow.damping.x);
        }
        if (math::abs(targetSpeedY) > math::abs(vel.stable.y)) {
            targetSpeedY = math::lerp(vel.stable.y, targetSpeedY, follow.damping.y);
        }

        vel.stable = {targetSpeedX, targetSpeedY};
        vel.stable *= {2, 2};

        // don't go too slow
        f32 minspeed = 1.91;  // min speed for rounding to not zero at 60fps
        if (vel.stable.x > 0 && vel.stable.x < minspeed) {
            vel.stable.x = minspeed;
        } else if (vel.stable.x < 0 && vel.stable.x > -minspeed) {
            vel.stable.x = -minspeed;
        }

        if (vel.stable.x == 0 || !isTargetMovingX) {
            follow.isMovingX = false;
        }

        if (vel.stable.y > 0 && vel.stable.y < minspeed) {
            vel.stable.y = minspeed;
        } else if (vel.stable.y < 0 && vel.stable.y > -minspeed) {
            vel.stable.y = -minspeed;
        } else if (vel.stable.y == 0) {
            follow.isMovingY = false;
        }

#ifndef NDEBUG
        ecs::Entity debugTargetTracker(follow.debugTargetTrackerID);
        ecs::Entity debugPositionTracker(follow.debugPositionTrackerID);

        debugTargetTracker.set(Transform(follow.currentTarget));
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
void FollowSystem::onEvent(evt::Death, ecs::Entity killedEntity) {
    std::vector<ecs::Entity> toRemove;
    for (auto& [entityid, entity] : FollowSystem::getEntities()) {
        if (entity.get<Follow>().targetEntityID == killedEntity.id()) {
            toRemove.push_back(entity);
        }
    }

    for (auto entity : toRemove) {
        entity.remove<Follow>();
    }
}

}  // namespace whal
