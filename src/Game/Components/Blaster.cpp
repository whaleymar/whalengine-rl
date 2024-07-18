#include "Blaster.h"
#include <raylib.h>

#include "Components/AnimUtil.h"
#include "Components/Animator.h"
#include "Components/Callback.h"
#include "Components/Draw.h"
#include "Components/Name.h"
#include "Components/ParticleEmitter.h"
#include "Components/Relationships.h"

#include "Events/Events.h"

#include "Settings.h"
#include "Sys/Event.h"
#include "Sys/InputHandler.h"
#include "Sys/System.h"
#include "Util/MathUtil.h"
#include "Util/Vector.h"

#include "Components/RigidBody.h"
#include "Components/Transform.h"
#include "Components/Velocity.h"

#include "Game/Entities/Projectile.h"
#include "whalECS/src/ECS.h"

// where a shot originates from, relative to shooter's transform
// TODO should be in component
const Vector2i SHOOT_OFFSET = {0, PIXELS_PER_TILE};

static bool IS_AIMING = false;

using namespace whal;

ecs::Entity createManaGauge(ecs::Entity attachedEntity) {
    auto entity = System::world->entity(false).value();
    auto _ = ecs::DeferActivate(entity);

    const Vector2i offsetTexels = {-TEXELS_PER_TILE, 2 * TEXELS_PER_TILE};
    entity.add(Transform2D(attachedEntity.get<Transform2D>().position + offsetTexels));
    entity.add(Attach(attachedEntity, offsetTexels, Attach::DirectionParam::UseFacingForOffset));
    entity.add(Name("Mana Gauge"));

    AnimInfo animInfo = {{"actor/mana-gauge", 0, 5, 0.0}};
    Animator animator;
    loadAnimations(animator, animInfo);

    entity.add(Draw(Sprite(Depth::Foreground1, animator.getFrame())));

    animator.brain = [](Animator& animator, ecs::Entity self) -> bool {
        auto& sprite = self.get<Draw>().getSprite();
        f32 unsquishStep = System::dt();
        sprite.scale = {approach(sprite.scale.x(), 1.0, unsquishStep), approach(sprite.scale.y(), 1.0, unsquishStep)};

        ecs::Entity owner = ecs::Entity(self.get<Attach>().targetEntityID);
        auto blaster = owner.get<Blaster>();

        s32 frameIx = animator.curFrameIx;
        s32 targetFrameIx = blaster.maxShots - blaster.shotsRemaining;

        if (frameIx != targetFrameIx) {
            animator.curFrameIx = targetFrameIx;
            if (targetFrameIx == 0) {
                sprite.scale = {1.2, 0.8};
            }
            sprite.color.a = 255;
            if (self.has<FadeOut>()) {
                self.remove<FadeOut>();
            }
            return true;
        } else if (targetFrameIx == 0 && sprite.scale.x() == 1.0f && sprite.color.a == 255 && !self.has<FadeOut>()) {
            self.add(FadeOut(0.2, 1.0, 0.5));
        }
        return false;
    };

    entity.add(animator);

    return entity;
}

void shootProjectile() {
    IS_AIMING = false;

    // slight delay for enabling movement so player can adjust arrow keys
    System::schedule.after([]() { System::input.enableMovement(); }, 0.2);
    // but allow jumping immediately
    // System::input.enableJumping();

    // Vector2i moveNormali = System::input.getMoveNormal();
    // System::eventMgr.triggerEvent(GameEvent::SHOOT_EVENT, moveNormali);

    for (auto& [entityid, entity] : ProjectileSystem::getEntitiesMutable()) {
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
        Vector2f moveNormal = directionToVector<f32>(blaster.aimDirection).norm();
        velocity = moveNormal * blaster.projectileSpeed;

        // auto totalVel = velocity + entity.get<Velocity>().total;
        auto totalVelocity = velocity;
        // const bool isDownwardAngle = totalVelocity.x() != 0 && totalVelocity.y() < 0;
        // Vector2f pushStrength = isDownwardAngle ? blaster.pushStrengthDownAngle : blaster.pushStrengthDefault;
        Vector2f pushStrength(blaster.pushStrength, blaster.pushStrength);
        makeProjectile(entityid, shotOrigin, totalVelocity, blaster.projectileLifetimeSeconds, blaster.explosionRadius, pushStrength);

        // push shooter in opposite direction of projectile
        if (entity.has<RocketJumping>() && entity.has<Velocity>()) {
            entity.get<Velocity>().stable += moveNormal * -1 * blaster.shotKnockback;
        }

        System::audio.playClip(Sfx::SHOTFIRED, 0.2);

        blaster.aimReticle->kill();
        blaster.aimReticle = Corrade::Containers::NullOpt;

        blaster.cooldownRemaining = blaster.cooldownSeconds;
        blaster.shotsRemaining--;
    }
}

void ProjectileSystem::onEvent(ButtonPressOrReleaseEvent, InputType input, bool isPress) {
    if (input == InputType::AIM) {
        if (isPress && !IS_AIMING) {
            IS_AIMING = true;
            addAimReticles();

            // deactivate movement controls; those keys are now for aiming
            System::input.disableMovement();
            // System::input.disableJumping();

        } else if (!isPress && IS_AIMING) {
            shootProjectile();
        }
    } else if (isPress) {
        switch (input) {
        case InputType::UP:
        case InputType::DOWN:
            mIsAimUpdateNeeded = true;
            return;
        case InputType::LEFT:
            updateFacingDirections(false);
            mIsAimUpdateNeeded = true;
            return;
        case InputType::RIGHT:
            updateFacingDirections(true);
            mIsAimUpdateNeeded = true;
            return;
        default:
            return;
        }
    }
}

void ProjectileSystem::addAimReticles() {
    // Vector2i aimDirection = System::input.getMoveNormal();
    Direction aimDirection = System::input.getDirection();
    for (auto [entityid, entity] : getEntitiesMutable()) {
        Blaster& blaster = entity.get<Blaster>();
        // if (blaster.cooldownRemaining > 0) {
        //     continue;
        // }

        auto childExpected = System::world->entity(false);
        if (childExpected.isExpected()) {
            auto child = childExpected.value();
            blaster.aimReticle = child;
            auto _ = ecs::DeferActivate(child);

            // if not holding any direction, start with facing direction
            Transform2D parentTrans = entity.get<Transform2D>();
            if (aimDirection == Direction::Neutral) {
                aimDirection = parentTrans.facing == Facing::Left ? Direction::W : Direction::E;
            }
            blaster.aimDirection = aimDirection;

            Vector2i offset = Vector2i(PIXELS_PER_TILE, PIXELS_PER_TILE) * directionToVector<s32>(aimDirection);
            Vector2i position = SHOOT_OFFSET + parentTrans.position + offset;
            child.add(Transform2D(position));
            child.add(Draw(DrawRect(BROWN)));
        }
    }
}

void ProjectileSystem::update() {
    f32 dt = System::dt();
    Direction aimDirection = System::input.getDirection();
    for (auto [entityid, entity] : getEntitiesMutable()) {
        Blaster& blaster = entity.get<Blaster>();

        if (blaster.cooldownRemaining > 0) {
            blaster.cooldownRemaining -= dt;
        }
        if (blaster.shotsRemaining < blaster.maxShots) {
            if (!entity.has<RigidBody>() || entity.get<RigidBody>().isGrounded) {
                blaster.shotsRemaining = blaster.maxShots;
            }
        }

        if (!blaster.aimReticle) {
            // not initialized
            continue;
        }
        Transform2D parentTrans = entity.get<Transform2D>();
        if (aimDirection != Direction::Neutral && mIsAimUpdateNeeded) {
            blaster.aimDirection = aimDirection;
        }

        Vector2i offset = Vector2i(PIXELS_PER_TILE, PIXELS_PER_TILE) * directionToVector<s32>(blaster.aimDirection);
        Vector2i position = SHOOT_OFFSET + parentTrans.position + offset;
        blaster.aimReticle->set(Transform2D(position));
    }
    mIsAimUpdateNeeded = false;
}

void ProjectileSystem::onAdd(const ecs::Entity entity) {
    // cannot add/remove components in IMonitor methods
    System::schedule.eventFlow({entity}).add([](ecs::Entity entity) { createManaGauge(entity); }, entity);
}

void ProjectileSystem::onRemove(const ecs::Entity entity) {
    auto blaster = entity.get<Blaster>();
    if (blaster.aimReticle) {
        blaster.aimReticle->kill();
    }
}

void ProjectileSystem::onUnpause() {
    if (IS_AIMING && !System::input.isOn(InputType::AIM)) {
        shootProjectile();
    }
}

void ProjectileSystem::updateFacingDirections(bool isFacingRight) {
    if (!IS_AIMING) {
        return;
    }
    for (auto [entityid, entity] : getEntitiesMutable()) {
        auto& trans = entity.get<Transform2D>();
        trans.facing = isFacingRight ? Facing::Right : Facing::Left;
    }
}

void RocketJumpingSystem::update() {
    using namespace whal;

    constexpr f32 AIRRES_STEP_MULTIPLIER = 0.5;
    const f32 dt = System::dt();
    const f32 step = dt * AIRRES_STEP_MULTIPLIER;

    for (auto [entityid, entity] : getEntitiesCopy()) {
        auto rb = entity.get<RigidBody>();
        auto& rocketJumpComponent = entity.get<RocketJumping>();

        if (rocketJumpComponent.stateTime > 0.5 && (rb.isGrounded || rb.isLanding)) {
            entity.remove<RocketJumping>();
        } else {
            rocketJumpComponent.stateTime += dt;
            f32 newAirResistanceValue = approach(rocketJumpComponent.newAirResistance, rocketJumpComponent.originalAirResistance, step);
            rocketJumpComponent.newAirResistance = newAirResistanceValue;
            rb.frictionMultiplier.e[1] = newAirResistanceValue;
            entity.set(rb);
        }
    }
}

void RocketJumpingSystem::onAdd(const ecs::Entity entity) {
    auto& rb = entity.get<RigidBody>();
    auto& rocketJumpComponent = entity.get<RocketJumping>();
    const f32 newAirResistanceValue = rocketJumpComponent.newAirResistance;
    rocketJumpComponent.originalAirResistance = rb.frictionMultiplier.y();  // save for later
    rb.frictionMultiplier = {rb.frictionMultiplier.x(), newAirResistanceValue};

    constexpr f32 waitBetweenSils = 0.1;
    constexpr f32 silLifetime = 1.5;
    if (entity.has<Draw>() && entity.get<Draw>().getTag() == Draw::DrawTag::Sprite) {
        u32 eventId = System::schedule.eventFlow({entity})
                          .add([](ecs::Entity e) { e.add(ParticleEmitter(WorldMaterial::Fire, CollisionDir::ALL, 50, 0)); }, entity)
                          .add(&makeSilhouetteFromSprite, entity, silLifetime, RED)
                          .addWait(waitBetweenSils)
                          .add(&makeSilhouetteFromSprite, entity, silLifetime, RED)
                          .addWait(waitBetweenSils)
                          .add(&makeSilhouetteFromSprite, entity, silLifetime, RED)
                          .addWait(waitBetweenSils)
                          .add(&makeSilhouetteFromSprite, entity, silLifetime, RED)
                          .getId();
        rocketJumpComponent.silhouetteEventId = eventId;
    }
}

void RocketJumpingSystem::onRemove(const ecs::Entity entity) {
    auto& rb = entity.get<RigidBody>();
    const auto rocketJumpComponent = entity.get<RocketJumping>();
    rb.frictionMultiplier.e[1] = rocketJumpComponent.originalAirResistance;  // restore saved value
    System::schedule.cancelEventFlow(rocketJumpComponent.silhouetteEventId);
    if (entity.has<ParticleEmitter>()) {
        System::schedule.eventFlow({entity}).add([](ecs::Entity e) { e.remove<ParticleEmitter>(); }, entity);
    }
}
