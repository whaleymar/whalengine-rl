#include "Blaster.h"
#include <raylib.h>

#include "ECS/Draw.h"
#include "Events/Events.h"
#include "Settings.h"
#include "Systems/Event.h"
#include "Systems/InputHandler.h"
#include "Systems/System.h"
#include "Util/Vector.h"

#include "ECS/RigidBody.h"
#include "ECS/Transform.h"
#include "ECS/Velocity.h"

#include "Game/Entities/Projectile.h"
#include "Game/Events.h"
#include "whalECS/src/ECS.h"

// where a shot originates from, relative to shooter's transform
const Vector2i SHOOT_OFFSET = {0, PIXELS_PER_TILE};

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

    for (auto& [entityid, entity] : ProjectileSystem::getEntitiesRef()) {
        Transform2D trans = entity.get<Transform2D>();

        // change where the projectile starts (relative to shooting entity)
        Vector2i shotOrigin = trans.position + SHOOT_OFFSET;
        // Vector2f moveNormal = closestOrdinalDirection(toFloatVec(target - shotOrigin).norm());

        Vector2f velocity;
        Blaster& blaster = entity.get<Blaster>();
        Vector2f moveNormal = toFloatVec(blaster.aimDirection);
        velocity = moveNormal * blaster.projectileSpeed;

        // auto totalVel = velocity + entity.get<Velocity>().total;
        auto totalVel = velocity;
        makeProjectile(entityid, shotOrigin, totalVel, blaster.projectileLifetimeSeconds, blaster.explosionRadius);

        // push shooter in opposite direction of projectile
        if (auto velOpt = entity.tryGet<Velocity>(); velOpt) {
            (*velOpt)->stable += moveNormal * -1 * blaster.shotKnockback;
        }

        System::audio.playClip(Sfx::SHOTFIRED, 0.2);

        blaster.aimReticle->kill();
        blaster.aimReticle = Corrade::Containers::NullOpt;
    }
}

void doShootEvent() {
    ProjectileSystem::setIsAiming(false);

    // slight delay for enabling movement so player can adjust arrow keys
    whal::System::schedule.after([]() { whal::System::input.enableMovement(); }, 0.2);
    // but allow jumping immediately
    whal::System::input.enableJumping();
    Vector2i moveNormal = whal::System::input.getMoveNormal();
    whal::System::eventMgr.triggerEvent(GameEvent::SHOOT_EVENT, moveNormal);
}

void onKeyPressOrRelease(whal::InputType input, bool isPress) {
    if (whal::System::isPaused()) {
        return;
    }
    if (input == whal::InputType::AIM) {
        if (isPress && !ProjectileSystem::getIsAiming()) {
            ProjectileSystem::setIsAiming(true);
            ProjectileSystem::addAimReticles();

            // deactivate movement controls; those keys are now for aiming
            whal::System::input.disableMovement();
            whal::System::input.disableJumping();

        } else if (!isPress && ProjectileSystem::getIsAiming()) {
            doShootEvent();
        }
    } else if (isPress) {
        switch (input) {
        case whal::InputType::LEFT:
            ProjectileSystem::updateFacingDirections(false);
            return;
        case whal::InputType::RIGHT:
            ProjectileSystem::updateFacingDirections(true);
            return;
        default:
            return;
        }
    }
}

ProjectileSystem::ProjectileSystem()
    : mBlasterEventListener(whal::EventListener<Vector2i>(&onBlasterFired)),
      mInputListener(whal::EventListener<whal::InputType, bool>(&onKeyPressOrRelease)) {
    whal::System::eventMgr.registerListener(GameEvent::SHOOT_EVENT, mBlasterEventListener);

    whal::System::eventMgr.registerListener(whal::Event::BUTTON_PRESSRELEASE, mInputListener);
}

void ProjectileSystem::addAimReticles() {
    Vector2i aimDirection = whal::System::input.getMoveNormal();
    for (auto [entityid, entity] : getEntitiesRef()) {
        auto childExpected = whal::System::world->entity(false);
        if (childExpected.isExpected()) {
            auto child = childExpected.value();
            Blaster& blaster = entity.get<Blaster>();
            blaster.aimReticle = child;
            auto _ = whal::ecs::DeferActivate(child);

            // if not holding any direction, start with facing direction
            whal::Transform2D parentTrans = entity.get<whal::Transform2D>();
            if (aimDirection.isZero()) {
                aimDirection.e[0] = parentTrans.facing == whal::Facing::Left ? -1 : 1;
            }
            blaster.aimDirection = aimDirection;

            Vector2i offset = Vector2i(PIXELS_PER_TILE, PIXELS_PER_TILE) * aimDirection;
            Vector2i position = SHOOT_OFFSET + parentTrans.position + offset;
            child.add(whal::Transform2D(position));
            child.add(whal::Draw(BROWN));
        }
    }
}

void ProjectileSystem::update() {
    if (!mIsAiming || whal::System::isPaused()) {
        return;
    }
    Vector2i aimDirection = whal::System::input.getMoveNormal();
    for (auto [entityid, entity] : getEntitiesRef()) {
        Blaster& blaster = entity.get<Blaster>();
        if (!blaster.aimReticle) {
            // not initialized
            continue;
        }
        whal::Transform2D parentTrans = entity.get<whal::Transform2D>();
        if (!aimDirection.isZero()) {
            blaster.aimDirection = aimDirection;
        }

        Vector2i offset = Vector2i(PIXELS_PER_TILE, PIXELS_PER_TILE) * blaster.aimDirection;
        Vector2i position = SHOOT_OFFSET + parentTrans.position + offset;
        blaster.aimReticle->set(whal::Transform2D(position));
    }
}

void ProjectileSystem::onRemove(whal::ecs::Entity entity) {
    auto blaster = entity.get<Blaster>();
    if (blaster.aimReticle) {
        blaster.aimReticle->kill();
    }
}

void ProjectileSystem::onUnpause() {
    if (getIsAiming() && !whal::System::input.isOn(whal::InputType::AIM)) {
        doShootEvent();
    }
}

void ProjectileSystem::updateFacingDirections(bool isFacingRight) {
    if (!mIsAiming || whal::System::isPaused()) {
        return;
    }
    for (auto [entityid, entity] : getEntitiesRef()) {
        auto& trans = entity.get<whal::Transform2D>();
        trans.facing = isFacingRight ? whal::Facing::Right : whal::Facing::Left;
    }
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
