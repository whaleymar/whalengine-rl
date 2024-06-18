#include "Blaster.h"

#include "Events/Events.h"
#include "Systems/Event.h"
#include "Systems/System.h"
#include "Util/Vector.h"

#include "ECS/RigidBody.h"
#include "ECS/Transform.h"
#include "ECS/Velocity.h"

#include "Game/Entities/Projectile.h"
#include "Game/Events.h"

Vector2f closestOrdinalDirection(Vector2f vecf) {
    vecf = vecf.norm();
    const f32 invRootTwo = 1.0f / std::sqrt(2.0f);
    auto tryUnitDir = [vecf](Vector2f& closest, f32& minDegreesAway, Vector2f other) {
        f32 otherDP = vecf.dot(other);
        f32 degreesAway = std::abs(std::acos(otherDP));
        if (degreesAway < minDegreesAway) {
            closest = other;
            minDegreesAway = degreesAway;
        }
    };

    if (vecf.x() == 0) {
        if (vecf.y() == 0) {
            return {1.0, 0.0};
        } else {
            return Vector2f(0.0, sign(vecf.y()));
        }
    } else if (vecf.x() < 0) {
        if (vecf.y() == 0) {
            return {-1.0, 0};
        } else if (vecf.y() < 0) {
            // SW quadrant
            Vector2f closest = Vector2f::unitLeft;
            f32 degreesAway = std::abs(std::acos(vecf.dot(closest)));

            tryUnitDir(closest, degreesAway, Vector2f::unitDown);
            tryUnitDir(closest, degreesAway, Vector2f(-1, -1) * invRootTwo);
            return closest;
        } else {
            // NW quadrant
            Vector2f closest = Vector2f::unitLeft;
            f32 degreesAway = std::abs(std::acos(vecf.dot(closest)));

            tryUnitDir(closest, degreesAway, Vector2f::unitUp);
            tryUnitDir(closest, degreesAway, Vector2f(-1, 1) * invRootTwo);
            return closest;
        }

    } else {
        if (vecf.y() == 0) {
            return {1.0, 0};
        } else if (vecf.y() < 0) {
            // SE quadrant
            Vector2f closest = Vector2f::unitRight;
            f32 degreesAway = std::abs(std::acos(vecf.dot(closest)));

            tryUnitDir(closest, degreesAway, Vector2f::unitDown);
            tryUnitDir(closest, degreesAway, Vector2f(1, -1) * invRootTwo);
            return closest;

        } else {
            // NE quadrant
            Vector2f closest = Vector2f::unitRight;
            f32 degreesAway = std::abs(std::acos(vecf.dot(closest)));

            tryUnitDir(closest, degreesAway, Vector2f::unitUp);
            tryUnitDir(closest, degreesAway, Vector2f(1, 1) * invRootTwo);
            return closest;
        }
    }
}

// void onBlasterFired(Vector2i target) {
void onBlasterFired(Vector2i moveNormali) {
    using namespace whal;
    if (System::isPaused()) {
        return;
    }

    Vector2f moveNormal = toFloatVec(moveNormali);
    for (auto& [entityid, entity] : ProjectileSystem::getEntitiesRef()) {
        Transform2D trans = entity.get<Transform2D>();

        // change where the projectile starts (relative to shooting entity)
        Vector2i offset = {0, PIXELS_PER_TILE};
        Vector2i shotOrigin = trans.position + offset;
        // Vector2f moveNormal = closestOrdinalDirection(toFloatVec(target - shotOrigin).norm());
        if (moveNormali.isZero()) {
            moveNormal.e[0] = trans.facing == Facing::Left ? -1 : 1;
        }

        Vector2f velocity;
        Blaster& blaster = entity.get<Blaster>();
        velocity = moveNormal * blaster.projectileSpeed;

        // auto totalVel = velocity + entity.get<Velocity>().total;
        auto totalVel = velocity;
        makeProjectile(shotOrigin, totalVel, blaster.projectileLifetimeSeconds, blaster.explosionRadius);

        // push shooter in opposite direction of projectile
        if (auto velOpt = entity.tryGet<Velocity>(); velOpt) {
            (*velOpt)->stable += moveNormal * -1 * blaster.shotKnockback;
        }

        System::audio.playClip(Sfx::SHOTFIRED, 0.2);
    }
}

void onKeyPressOrRelease(whal::InputType input, bool isPress) {
    if (input != whal::InputType::AIM) {
        return;
    }

    if (isPress) {
        // deactivate movement controls; those keys are now for aiming
        whal::System::input.disableMovement();
        whal::System::input.disableJumping();

    } else {
        // slight delay for enabling movement so player can adjust arrow keys
        whal::System::schedule.after([]() { whal::System::input.enableMovement(); }, 0.2);
        // but allow jumping immediately
        whal::System::input.enableJumping();
        Vector2i moveNormal = whal::System::input.getMoveNormal();
        whal::System::eventMgr.triggerEvent(GameEvent::SHOOT_EVENT, moveNormal);
    }
}

ProjectileSystem::ProjectileSystem()
    : mBlasterEventListener(whal::EventListener<Vector2i>(&onBlasterFired)),
      mInputListener(whal::EventListener<whal::InputType, bool>(&onKeyPressOrRelease)) {
    whal::System::eventMgr.registerListener(GameEvent::SHOOT_EVENT, mBlasterEventListener);

    whal::System::eventMgr.registerListener(whal::Event::BUTTON_PRESSRELEASE, mInputListener);
}

void onRocketJumperLands(whal::ecs::Entity entity) {
    if (entity.has<RocketJumping>()) {
        entity.remove<RocketJumping>();
    }
}

RocketJumpingSystem::RocketJumpingSystem() : mLandingEventListener(whal::EventListener<whal::ecs::Entity>(&onRocketJumperLands)) {
    whal::System::eventMgr.registerListener(whal::Event::LANDING, mLandingEventListener);
}
void RocketJumpingSystem::onAdd(const whal::ecs::Entity entity) {
    auto& rb = entity.get<whal::RigidBody>();
    entity.get<RocketJumping>().prevFrictionMultiplier = rb.frictionMultiplier;  // save for later
    rb.frictionMultiplier = {rb.frictionMultiplier.x(), 0};
}

void RocketJumpingSystem::onRemove(const whal::ecs::Entity entity) {
    auto& rb = entity.get<whal::RigidBody>();
    rb.frictionMultiplier = entity.get<RocketJumping>().prevFrictionMultiplier;  // restore saved value
}
