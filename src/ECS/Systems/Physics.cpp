#include "Physics.h"

#include <cmath>
#include <functional>

#include "ECS/PlayerControl.h"
#include "ECS/Systems/CollisionManager.h"
#include "ECS/TriggerZone.h"
#include "Events/Events.h"
#include "Physics/HitInfo.h"
#include "Settings.h"

#include "ECS/Collision.h"
#include "ECS/RigidBody.h"
#include "ECS/Tags.h"
#include "ECS/Transform.h"
#include "ECS/Velocity.h"

#include "Systems/System.h"
#include "Util/MathUtil.h"
#include "Util/Vector.h"

namespace whal {

constexpr f32 GRAVITY = 280;

constexpr f32 FRICTION_GROUND = 240;
constexpr f32 FRICTION_AIR = 200;
constexpr f32 MOVE_EPSILON = 0.1;

constexpr f32 JUMP_PEAK_GRAVITY_MULT = 0.5;
constexpr f32 JUMP_PEAK_SPEED_MAX = -28;  // once Y velocity is below this, no longer considered "jumping"

using BoundCollisionCallback = std::function<void()>;

void applyGravity(Velocity& velocity, f32 dt, f32 gravityMultiplier, bool isJumping) {
    bool isInJumpPeak = isJumping && isBetween(velocity.total.y(), JUMP_PEAK_SPEED_MAX, 0.0f);
    f32 peakMultiplier = 1 - static_cast<f32>(isInJumpPeak) * (1 - JUMP_PEAK_GRAVITY_MULT);
    velocity.stable.e[1] = approach(velocity.stable.y(), gravityMultiplier * TERMINAL_VELOCITY_Y, gravityMultiplier * GRAVITY * peakMultiplier * dt);
}

void applyFriction(Vector2f& velocity, f32 frictionMultiplier) {
    velocity.e[0] = approach(velocity.x(), 0, frictionMultiplier);
}

// Any type of collision (regular, push, carry) is emitted as an event and received here.
// If the moving entity or the other entity have callbacks, they're added to a queue and called once everything has moved.
// If both entities are moving, this might get called twice for the same pair of entities.
// So I have a helper function that makes sure they're unique
void PhysicsSystem::onEvent(CollisionEvent, ecs::Entity movingEntity, HitInfo hitinfo) {
    auto& queue = PhysicsSystem::getCollisionCallbackQueue();
    auto& movingCollider = movingEntity.get<Collider>();
    auto& otherCollider = hitinfo.getOther().get<Collider>();

    auto addIfUnique = [](std::vector<std::pair<ecs::Entity, BoundCollisionCallback>>& entityList, ecs::Entity callbackOwner, ecs::Entity other,
                          Collider* callbackOwnerCollider, Collider* otherCollider, Vector2i moveNormal) {
        for (auto [entity, _] : entityList) {
            if (entity == other) {
                return;
            }
        }
        // TODO it's weird i'm not passing hitinfo here. The callback should take hitinfo instead of moveNormal
        BoundCollisionCallback boundFunc =
            std::bind(callbackOwnerCollider->getOnCollisionEnter(), callbackOwner, other, callbackOwnerCollider, otherCollider, moveNormal);
        entityList.push_back({other, boundFunc});
    };

    if (movingCollider.getOnCollisionEnter() != nullptr) {
        addIfUnique(queue[movingEntity], movingEntity, hitinfo.getOther(), &movingCollider, &otherCollider, hitinfo.toVec());
    }
    if (otherCollider.getOnCollisionEnter() != nullptr) {
        addIfUnique(queue[hitinfo.getOther()], hitinfo.getOther(), movingEntity, &otherCollider, &movingCollider, hitinfo.toVec() * -1);
    }
}

void PhysicsSystem::update() {
    // sync collider in case position changed in another system
    // is a little inefficient to do it this way (vs separating the systems)
    for (auto& [entityid, entity] : getEntitiesRef()) {
        Transform2D& trans = entity.get<Transform2D>();
        if (!trans.isManuallyMoved) {
            continue;
        }

        trans.isManuallyMoved = false;
        if (auto colliderOpt = entity.tryGet<Collider>(); colliderOpt) {
            QuadTreeSystem::updatePosition(entity, &(*colliderOpt)->getShapeMutable(), trans);
        }

        if (auto precisePositionOpt = entity.tryGet<PrecisePosition>(); precisePositionOpt) {
            (*precisePositionOpt)->position = toFloatVec(trans.position);
        }
    }

    std::vector<ecs::Entity> allColliderEntities;
    for (auto& [entityid, entity] : getEntitiesRef()) {
        f32 dt;
        // camera move normally unless pause menu is active
        if (entity.has<Camera>()) {
            dt = System::dt.getUnmodified();
        } else {
            dt = System::dt();
        }

        auto rbOpt = entity.tryGet<RigidBody>();
        Vector2f frictionMultiplier = {1, 1};
        if (rbOpt) {
            // DOESNT WORK TODO
            // f32 justLandedMultiplier = std::max(1.0f, static_cast<f32>((*rbOpt)->framesSinceLanding) / 60.0f);
            // frictionMultiplier = (*rbOpt)->frictionMultiplier * Vector2f(justLandedMultiplier, 1.0);
            // (*rbOpt)->framesSinceLanding++;
            frictionMultiplier = (*rbOpt)->frictionMultiplier;
        }

        const f32 frictionStepGround = dt * FRICTION_GROUND * frictionMultiplier.x();
        const f32 frictionStepAir = dt * FRICTION_AIR * frictionMultiplier.y();
        const f32 gravityStep = dt * GRAVITY * 3;
        Transform2D& trans = entity.get<Transform2D>();
        Velocity& vel = entity.get<Velocity>();

        // if impulse ends, use residual
        Vector2f impulse = vel.impulse;
        if (!impulse.x() && !isNearZero(vel.residualImpulse.x(), MOVE_EPSILON)) {
            impulse.e[0] += vel.residualImpulse.x();
        }
        if (!impulse.y() && !isNearZero(vel.residualImpulse.y(), MOVE_EPSILON)) {
            impulse.e[1] += vel.residualImpulse.y();
        }

        const Vector2f totalVelocity = vel.stable + impulse;
        const Vector2f move = {totalVelocity.x() * dt * PIXELS_PER_TEXEL, totalVelocity.y() * dt * PIXELS_PER_TEXEL};

        vel.residualImpulse = {approach(impulse.x(), 0, frictionStepGround), approach(impulse.y(), 0, gravityStep)};
        vel.impulse = {0, 0};
        vel.total = totalVelocity;

        auto jumpControl = entity.tryGet<Jumper>();
        auto colliderOpt = entity.tryGet<Collider>();

        // ----------------------------------------------------------------
        // UPDATE POSITION
        if (colliderOpt) {
            (*colliderOpt)->move(move, nullptr, bool(rbOpt), false, false, bool(rbOpt));
            allColliderEntities.push_back(entity);
        } else {
            if (auto precisePositionOpt = entity.tryGet<PrecisePosition>(); precisePositionOpt) {
                (*precisePositionOpt)->position += move;
                trans.position = Vector2i(std::round((*precisePositionOpt)->position.x()), std::round((*precisePositionOpt)->position.y()));
                // if (move.len() < 0.1) {
                // clamp precise position to integer coordinates if we're not moving
                // (*precisePositionOpt)->position = toFloatVec(trans.position);
                // }
            } else {
                // store remainder for stuff that moves less than 1px per frame. This is done in collider.move for colliders.
                Vector2i moveRounded = Vector2i(std::round(move.x()), std::round(move.y()));
                auto remainder = move - toFloatVec(moveRounded);
                vel.residualImpulse += remainder;
                trans.position += moveRounded;
            }

            if (entity.has<Trigger>()) {
                auto trigger = entity.get<Trigger>();
                Transform2D adjustedTransform = Transform2D(trans.position + trigger.offset);
                trigger.shape.setPosition(adjustedTransform);
                entity.set(trigger);
            }
        }
        // ----------------------------------------------------------------

        // ----------------------------------------------------------------
        // UPDATE VELOCITY
        if (rbOpt) {
            // friction
            if (vel.stable.x()) {
                if ((*rbOpt)->isGrounded) {
                    applyFriction(vel.stable, frictionStepGround);
                } else {
                    applyFriction(vel.stable, frictionStepAir);
                    vel.residualImpulse.e[0] = approach(impulse.x(), 0, frictionStepAir);  // recalced
                }
            }

            // gravity
            if (!(*rbOpt)->isGrounded) {
                if (totalVelocity.y() < JUMP_PEAK_SPEED_MAX && jumpControl) {
                    (*jumpControl)->isJumping = false;
                }

                if (jumpControl) {
                    if (totalVelocity.y() < JUMP_PEAK_SPEED_MAX) {
                        // falling == not jumping
                        // a little lower than 0 while applying reduced gravity
                        (*jumpControl)->isJumping = false;
                    }
                    applyGravity(vel, dt, (*rbOpt)->gravityMultiplier, (*jumpControl)->isJumping);
                } else {
                    applyGravity(vel, dt, (*rbOpt)->gravityMultiplier, false);
                }

                (*rbOpt)->isLanding = false;

                // } else {
                //     if (totalVelocity.y() < 0 && (!jumpControl || !(*jumpControl)->isJumping)) {
                //         // zero y velocity when grounded and not trying to jump, otherwise entity falls at terminal velocity after walking off
                //         platform vel.stable.e[1] = 0;
                //     }
            }
        }

        // ----------------------------------------------------------------
    }

    // sync collider positions to transform components.
    // done after all movement in case things get pushed by others
    for (auto entity : allColliderEntities) {
        Transform2D& trans = entity.get<Transform2D>();
        auto shape = entity.get<Collider>().getShape();
        trans.position = centerToTrans(shape.getPosition(), shape.getHalf(), trans.rotationDegrees);
        if (auto precisePositionOpt = entity.tryGet<PrecisePosition>(); precisePositionOpt) {
            (*precisePositionOpt)->position = toFloatVec(trans.position);
        }

        if (entity.has<Trigger>()) {
            auto trigger = entity.get<Trigger>();
            Transform2D adjustedTransform = Transform2D(trans.position + trigger.offset);
            trigger.shape.setPosition(adjustedTransform);
            entity.set(trigger);
        }
    }

    // do collision callbacks
    for (auto& [callbackEntity, hitList] : mCollisionCallbackQueue) {
        for (auto& [otherEntity, callback] : hitList) {
            callback();
        }
    }
    mCollisionCallbackQueue.clear();
}

}  // namespace whal
