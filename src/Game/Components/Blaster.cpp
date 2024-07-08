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
#include "whalECS/src/ECS.h"

// where a shot originates from, relative to shooter's transform
const Vector2i SHOOT_OFFSET = {0, PIXELS_PER_TILE};

void shootProjectile() {
    using namespace whal;

    ProjectileSystem::setIsAiming(false);

    // slight delay for enabling movement so player can adjust arrow keys
    System::schedule.after([]() { System::input.enableMovement(); }, 0.2);
    // but allow jumping immediately
    System::input.enableJumping();

    // Vector2i moveNormali = System::input.getMoveNormal();
    // System::eventMgr.triggerEvent(GameEvent::SHOOT_EVENT, moveNormali);

    for (auto& [entityid, entity] : ProjectileSystem::getEntitiesRef()) {
        Blaster& blaster = entity.get<Blaster>();
        if (blaster.cooldownRemaining > 0 || blaster.shotsRemaining == 0) {
            if (blaster.aimReticle) {
                blaster.aimReticle->kill();
                blaster.aimReticle = Corrade::Containers::NullOpt;
            }
            continue;
        }

        Transform2D trans = entity.get<Transform2D>();

        // change where the projectile starts (relative to shooting entity)
        Vector2i shotOrigin = trans.position + SHOOT_OFFSET;
        // Vector2f moveNormal = closestOrdinalDirection(toFloatVec(target - shotOrigin).norm());

        Vector2f velocity;
        Vector2f moveNormal = toFloatVec(blaster.aimDirection);
        velocity = moveNormal * blaster.projectileSpeed;

        // auto totalVel = velocity + entity.get<Velocity>().total;
        auto totalVelocity = velocity;
        const bool isDownwardAngle = totalVelocity.x() != 0 && totalVelocity.y() < 0;
        Vector2f pushStrength = isDownwardAngle ? blaster.pushStrengthDownAngle : blaster.pushStrengthDefault;
        makeProjectile(entityid, shotOrigin, totalVelocity, blaster.projectileLifetimeSeconds, blaster.explosionRadius, pushStrength);

        // push shooter in opposite direction of projectile
        if (auto velOpt = entity.tryGet<Velocity>(); velOpt) {
            (*velOpt)->stable += moveNormal * -1 * blaster.shotKnockback;
        }

        System::audio.playClip(Sfx::SHOTFIRED, 0.2);

        blaster.aimReticle->kill();
        blaster.aimReticle = Corrade::Containers::NullOpt;

        blaster.cooldownRemaining = blaster.cooldownSeconds;
        blaster.shotsRemaining--;
    }
}

void ProjectileSystem::onEvent(whal::ButtonPressOrReleaseEvent, whal::InputType input, bool isPress) {
    if (input == whal::InputType::AIM) {
        if (isPress && !getIsAiming()) {
            setIsAiming(true);
            addAimReticles();

            // deactivate movement controls; those keys are now for aiming
            whal::System::input.disableMovement();
            whal::System::input.disableJumping();

        } else if (!isPress && ProjectileSystem::getIsAiming()) {
            shootProjectile();
        }
    } else if (isPress) {
        switch (input) {
        case whal::InputType::UP:
        case whal::InputType::DOWN:
            mIsAimUpdateNeeded = true;
            return;
        case whal::InputType::LEFT:
            updateFacingDirections(false);
            mIsAimUpdateNeeded = true;
            return;
        case whal::InputType::RIGHT:
            updateFacingDirections(true);
            mIsAimUpdateNeeded = true;
            return;
        default:
            return;
        }
    }
}

void ProjectileSystem::addAimReticles() {
    Vector2i aimDirection = whal::System::input.getMoveNormal();
    for (auto [entityid, entity] : getEntitiesRef()) {
        Blaster& blaster = entity.get<Blaster>();
        // if (blaster.cooldownRemaining > 0) {
        //     continue;
        // }

        auto childExpected = whal::System::world->entity(false);
        if (childExpected.isExpected()) {
            auto child = childExpected.value();
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
    // if (!mIsAiming) {
    //     return;
    // }
    f32 dt = whal::System::dt();
    Vector2i aimDirection = whal::System::input.getMoveNormal();
    for (auto [entityid, entity] : getEntitiesRef()) {
        Blaster& blaster = entity.get<Blaster>();

        if (blaster.cooldownRemaining > 0) {
            blaster.cooldownRemaining -= dt;
        }
        if (blaster.shotsRemaining < blaster.maxShots) {
            if (!entity.has<whal::RigidBody>() || entity.get<whal::RigidBody>().isGrounded) {
                blaster.shotsRemaining = blaster.maxShots;
            }
        }

        if (!blaster.aimReticle) {
            // not initialized
            continue;
        }
        whal::Transform2D parentTrans = entity.get<whal::Transform2D>();
        if (!aimDirection.isZero() && mIsAimUpdateNeeded) {
            blaster.aimDirection = aimDirection;
        }

        Vector2i offset = Vector2i(PIXELS_PER_TILE, PIXELS_PER_TILE) * blaster.aimDirection;
        Vector2i position = SHOOT_OFFSET + parentTrans.position + offset;
        blaster.aimReticle->set(whal::Transform2D(position));
    }
    mIsAimUpdateNeeded = false;
}

void ProjectileSystem::onRemove(whal::ecs::Entity entity) {
    auto blaster = entity.get<Blaster>();
    if (blaster.aimReticle) {
        blaster.aimReticle->kill();
    }
}

void ProjectileSystem::onUnpause() {
    if (getIsAiming() && !whal::System::input.isOn(whal::InputType::AIM)) {
        shootProjectile();
    }
}

void ProjectileSystem::updateFacingDirections(bool isFacingRight) {
    if (!mIsAiming) {
        return;
    }
    for (auto [entityid, entity] : getEntitiesRef()) {
        auto& trans = entity.get<whal::Transform2D>();
        trans.facing = isFacingRight ? whal::Facing::Right : whal::Facing::Left;
    }
}

void RocketJumpingSystem::update() {
    using namespace whal;

    for (auto [entityid, entity] : getEntitiesCopy()) {
        const auto rb = entity.get<RigidBody>();
        const auto rocketJumpComponent = entity.get<RocketJumping>();

        if (rb.isGrounded || rb.isLanding) {
            entity.remove<RocketJumping>();
        }
    }
}

void RocketJumpingSystem::onAdd(const whal::ecs::Entity entity) {
    auto& rb = entity.get<whal::RigidBody>();
    auto& rocketJumpComponent = entity.get<RocketJumping>();
    rocketJumpComponent.prevFrictionMultiplier = rb.frictionMultiplier;  // save for later
    rb.frictionMultiplier = {rb.frictionMultiplier.x(), 0};

    constexpr f32 waitBetweenSils = 0.1;
    constexpr f32 silLifetime = 1.5;
    if (entity.has<whal::Sprite>()) {
        u32 eventId = whal::System::schedule.eventFlow({entity})
                          .add(&whal::makeSilhouetteFromSprite, entity, silLifetime, RED)
                          .addWait(waitBetweenSils)
                          .add(&whal::makeSilhouetteFromSprite, entity, silLifetime, RED)
                          .addWait(waitBetweenSils)
                          .add(&whal::makeSilhouetteFromSprite, entity, silLifetime, RED)
                          .addWait(waitBetweenSils)
                          .add(&whal::makeSilhouetteFromSprite, entity, silLifetime, RED)
                          .getId();
        rocketJumpComponent.silhouetteEventId = eventId;
    }
}

void RocketJumpingSystem::onRemove(const whal::ecs::Entity entity) {
    auto& rb = entity.get<whal::RigidBody>();
    const auto rocketJumpComponent = entity.get<RocketJumping>();
    rb.frictionMultiplier = rocketJumpComponent.prevFrictionMultiplier;  // restore saved value
    whal::System::schedule.cancelEventFlow(rocketJumpComponent.silhouetteEventId);
}
