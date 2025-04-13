#include "PhysicsSystem.h"

#include <cmath>

#include "Events/Events.h"
#include "Physics/HitInfo.h"
#include "Physics/MaterialData.h"
#include "Physics/Shapes.h"
#include "Settings.h"
#include "Sys/Time.h"
#include "Systems/ColliderSystem.h"

#include "Components/Collider.h"
#include "Components/PlayerControl.h"
#include "Components/RigidBody.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Components/Velocity.h"

#include "Sys/System.h"
#include "Util/MathUtil.h"
#include "Util/Vector.h"

namespace whal {

using CallbackMap = std::unordered_map<ecs::Entity, std::vector<std::pair<ecs::Entity, Vector2i>>, ecs::EntityHash>;

// this gets cleared at the beginning of PhsyicsSystem::update
static CallbackMap S_CALLBACK_QUEUE;

void applyGravity(Velocity& velocity, f32 dt, f32 gravityMultiplier, bool isJumping, const PhysicsParams& params) {
    bool isInJumpPeak = isJumping && math::isBetween(velocity.total.y, params.jumpPeakSpeedMax, 0.0f);
    f32 peakMultiplier = 1 - static_cast<f32>(isInJumpPeak) * (1 - params.jumpPeakGravityMult);
    velocity.stable.y = math::approach(velocity.stable.y, gravityMultiplier * params.terminalVelocityY,
                                       math::abs(gravityMultiplier) * params.gravity * peakMultiplier * dt);
}

void applyFriction(Vector2f& velocity, f32 frictionMultiplier) {
    velocity.x = math::approach(velocity.x, 0, frictionMultiplier);
}

PhysicsSystem::PhysicsSystem() {
    if (!World.has<PhysicsParams>()) {
        World.add<PhysicsParams>();
    }
}

// Any type of collision (regular, push, carry) is emitted as an event and received here.
// If the moving entity or the other entity have callbacks, they're added to a queue and called once everything has moved.
// If both entities are moving, this might get called twice for the same pair of entities.
// So I have a helper function that makes sure callbacks are called exactly once per collision
void PhysicsSystem::onEvent(evt::Collision, ecs::Entity movingEntity, HitInfo hitinfo) {
    auto addIfUnique = [](std::vector<std::pair<ecs::Entity, Vector2i>>& entityList, ecs::Entity other, Vector2i moveNormal) {
        for (auto [entity, _] : entityList) {
            if (entity == other) {
                return;
            }
        }
        entityList.push_back({other, moveNormal});
    };

    if (movingEntity.get<Collider>().getOnCollisionEnter() != nullptr) {
        addIfUnique(S_CALLBACK_QUEUE[movingEntity], hitinfo.getOther(), hitinfo.toVec());
    }
    if (hitinfo.getOther().get<Collider>().getOnCollisionEnter() != nullptr) {
        addIfUnique(S_CALLBACK_QUEUE[hitinfo.getOther()], movingEntity, hitinfo.toVec() * -1);
    }
}
// syncs collider in case position changed in another system
static void syncColliders(const std::unordered_map<ecs::EntityID, ecs::Entity>& physicsEntities) {
    for (auto& [entityid, entity] : physicsEntities) {
        Transform& trans = entity.get<Transform>();
        const bool isManuallyMoved = trans.isManuallyMoved;
        trans.isManuallyMoved = false;

        if (!entity.has<Collider>()) {
            continue;
        }

        auto& collider = entity.get<Collider>();
        if (isManuallyMoved) {
            // Sync collider position without checking collision
            if (collider.getShape().getPosition() != trans.apply2D(collider.getOffset())) {
                ColliderSystem::updatePosition(entity, collider.getShapeMutable(), trans, collider.getOffset());
            }

        } else {
            // Move collider within physics engine
            const Vector2i targetColliderPosition = trans.apply2D(collider.getOffset());
            if (collider.getShape().getPosition() != targetColliderPosition) {
                const Vector2f toMove = (targetColliderPosition - collider.getShape().getPosition()).as<f32>();

                // IsManualMove=true, so transform and QuadTree are synced automatically
                collider.move(toMove, nullptr, false, true);
            }
        }
    }
}

// RESEARCH (bug i will eventually run into)
// if a parent and child entity both have colliders, the parent entity moving will not move the child in the quad tree and it will crash
void PhysicsSystem::update() {
    if (System::isEnginePaused()) {
        return;
    }

    S_CALLBACK_QUEUE.clear();
    // ColliderSystem::syncColliders();
    syncColliders(getEntities());
    const PhysicsParams& physicsParams = World.get<PhysicsParams>();

    std::vector<ecs::Entity> allColliderEntities;
    for (auto& [entityid, entity] : getEntities()) {
        f32 dt;
        // camera move normally unless pause menu is active
        if (entity.has<IgnoreTimeModifiers>()) {
            dt = Time.getUnmodified();
        } else {
            dt = Time.dt();
        }

        RigidBody* rbOpt = entity.tryGet<RigidBody>();
        Vector2f frictionMultiplier = {1, 1};
        if (rbOpt) {
            frictionMultiplier = rbOpt->frictionMultiplier;
        }

        const f32 frictionStepGround = dt * physicsParams.frictionGround * frictionMultiplier.x;
        const f32 frictionStepAir = dt * physicsParams.frictionAir * frictionMultiplier.y;
        const f32 gravityStep = dt * physicsParams.gravity * 3;
        Transform& trans = entity.get<Transform>();
        Velocity& vel = entity.get<Velocity>();

        // if impulse ends, use residual
        Vector2f impulse = vel.impulse;
        if (!impulse.x && !math::isNearZero(vel.residualImpulse.x, physicsParams.moveEpsilon)) {
            impulse.x += vel.residualImpulse.x;
        }
        if (!impulse.y && !math::isNearZero(vel.residualImpulse.y, physicsParams.moveEpsilon)) {
            impulse.y += vel.residualImpulse.y;
        }

        const Vector2f totalVelocity = vel.stable + impulse;
        const Vector2f move = totalVelocity * dt;

        vel.residualImpulse = {math::approach(impulse.x, 0, frictionStepGround), math::approach(impulse.y, 0, gravityStep)};
        vel.impulse = {0, 0};
        vel.total = totalVelocity;

        const Jumper* jumpControlOpt = entity.tryGet<Jumper>();

        // ----------------------------------------------------------------
        // UPDATE POSITION
        if constexpr (WORLD_TYPE == WorldType2D::SideScroller) {
            if (entity.has<Collider>()) {
                entity.get<Collider>().move(move, nullptr, bool(rbOpt), false, false, bool(rbOpt));
                allColliderEntities.push_back(entity);
            } else {
                trans.translate(move, entity);
            }
        } else {
            // for top-down games, anything with a RigidBody has its Y velocity converted to the Z axis
            if (!rbOpt && !entity.has<Particle>()) {
                if (entity.has<Collider>()) {
                    entity.get<Collider>().move(move, nullptr, bool(rbOpt), false, false, bool(rbOpt));
                    allColliderEntities.push_back(entity);
                } else {
                    trans.translate(move, entity);
                }
            } else {
                Vector2f moveXOnly = {move.x, 0.0f};
                Collider* colliderOpt = entity.tryGet<Collider>();

                if (colliderOpt) {
                    colliderOpt->move(moveXOnly, nullptr);
                    allColliderEntities.push_back(entity);
                } else {
                    trans.translate(moveXOnly, entity);
                }

                if (!math::isNearZero(move.y, 0.001)) {
                    f32 newFloatHeight = move.y + trans.floatHeight;
                    // don't ground or zero y vel for things that float upward
                    if (newFloatHeight <= 0.0f && (!rbOpt || rbOpt->gravityMultiplier > 0.0f)) {
                        if (rbOpt) {
                            // if the RB was not grounded last frame & there is a collider & the Y velocity is more than 1px/sec, then bounce using
                            // the collider material
                            if (!rbOpt->isGrounded && colliderOpt && !math::isNearZero(vel.stable.y, 1.0)) {
                                f32 bounce = MaterialData::get(colliderOpt->getMaterial()).bounciness;
                                vel.stable.y *= -1.0f * bounce;
                            } else {
                                rbOpt->isGrounded = true;
                            }
                        }
                        newFloatHeight = 0.0f;
                        if (vel.stable.y < 0.0f) {
                            vel.stable.y = 0.0f;
                        }
                    }
                    trans.setFloatHeight(newFloatHeight, entity);
                }
            }
        }

        // ----------------------------------------------------------------
        // UPDATE VELOCITY
        if (rbOpt) {
            // friction
            if (vel.stable.x) {
                if (rbOpt->isGrounded) {
                    applyFriction(vel.stable, frictionStepGround);
                } else {
                    applyFriction(vel.stable, frictionStepAir);
                    vel.residualImpulse.x = math::approach(impulse.x, 0, frictionStepAir);  // recalced
                }
            }

            // gravity
            if (!rbOpt->isGrounded) {
                if (jumpControlOpt) {
                    auto& jumpControl = entity.get<Jumper>();
                    if (totalVelocity.y < physicsParams.jumpPeakSpeedMax) {
                        // falling == not jumping
                        // a little lower than 0 while applying reduced gravity
                        jumpControl.isJumping = false;
                    }
                    applyGravity(vel, dt, rbOpt->gravityMultiplier, jumpControl.isJumping, physicsParams);
                } else {
                    applyGravity(vel, dt, rbOpt->gravityMultiplier, false, physicsParams);
                }

                rbOpt->isLanding = false;
            }
        }

        // ----------------------------------------------------------------
    }

    // sync other components with collider positions
    // done after all movement in case things get pushed by others
    for (auto entity : allColliderEntities) {
        entity.get<Collider>().updateEntityPosition();
    }

    // do collision callbacks
    for (auto& [callbackEntity, hitList] : S_CALLBACK_QUEUE) {
        for (auto& [otherEntity, hitNormal] : hitList) {
            callbackEntity.get<Collider>().getOnCollisionEnter()(callbackEntity, otherEntity, hitNormal);
        }
    }
}

void RotationPhysicsSystem::update() {
    const f32 dt = Time.dt();
    for (auto [entityid, entity] : getEntities()) {
        const auto angularVelocity = entity.get<AngularVelocity>();

        // multiply by -1 so rotations are clockwise by default
        const f32 toAdd = -1.0f * 360.0f * angularVelocity.rotationsPerSecond * dt;
        entity.get<Transform>().rotate(toAdd, entity);
    }
}

}  // namespace whal
