#include "Physics.h"

#include <cmath>
#include <functional>

#include "Game/Events.h"
#include "Settings.h"

#include "ECS/Collision.h"
#include "ECS/RigidBody.h"
#include "ECS/Tags.h"
#include "ECS/Transform.h"
#include "ECS/Velocity.h"

#include "Systems/PauseMenu.h"
#include "Systems/System.h"
#include "Util/MathUtil.h"
#include "Util/Vector.h"

namespace whal {

constexpr f32 GRAVITY = 280;

constexpr f32 FRICTION_GROUND = 240;
constexpr f32 FRICTION_AIR = 200;
// constexpr f32 FRICTION_AIR = 80.0;
constexpr f32 MOVE_EPSILON = 0.1;

constexpr f32 JUMP_PEAK_GRAVITY_MULT = 0.5;
constexpr f32 JUMP_PEAK_SPEED_MAX = -28;  // once Y velocity is below this, no longer considered "jumping"

constexpr s32 MOMENTUM_COOLDOWN_FRAMES = 10;

using BoundCollisionCallback = std::function<void()>;

void applyGravity(Velocity& velocity, f32 dt, bool isJumping) {
    bool isInJumpPeak = isJumping && isBetween(velocity.total.y(), JUMP_PEAK_SPEED_MAX, 0.0f);
    f32 peakMultiplier = 1 - static_cast<f32>(isInJumpPeak) * (1 - JUMP_PEAK_GRAVITY_MULT);
    velocity.stable.e[1] = approach(velocity.stable.y(), TERMINAL_VELOCITY_Y, GRAVITY * peakMultiplier * dt);
}

void applyFriction(Vector2f& velocity, f32 frictionMultiplier) {
    velocity.e[0] = approach(velocity.x(), 0, frictionMultiplier);
}

void addIfUnique(std::vector<std::pair<ecs::Entity, BoundCollisionCallback>>& entityList, ecs::Entity callbackOwner, ecs::Entity other,
                 IUseCollision* callbackOwnerCollider, IUseCollision* otherCollider, Vector2i moveNormal) {
    for (auto [entity, _] : entityList) {
        if (entity == other) {
            return;
        }
    }

    BoundCollisionCallback boundFunc =
        std::bind(callbackOwnerCollider->getOnCollisionEnter(), callbackOwner, other, callbackOwnerCollider, otherCollider, moveNormal);
    entityList.push_back({other, boundFunc});
}

void PhysicsSystem::update() {
    std::vector<ecs::Entity> allActors;
    std::vector<ecs::Entity> allSemiSolids;

    // sync collider in case position changed in another system
    // is a little inefficient to do it this way (vs separating the systems)
    for (auto& [entityid, entity] : getEntitiesRef()) {
        Transform2D& trans = entity.get<Transform2D>();
        if (!trans.isManuallyMoved) {
            continue;
        }

        trans.isManuallyMoved = false;
        if (std::optional<ActorCollider*> actor = entity.tryGet<ActorCollider>(); actor) {
            actor.value()->setPositionFromBottom(trans.position);
        } else if (std::optional<SolidCollider*> solid = entity.tryGet<SolidCollider>(); solid) {
            solid.value()->setPositionFromBottom(trans.position);
        } else if (std::optional<SemiSolidCollider*> semisolid = entity.tryGet<SemiSolidCollider>(); semisolid) {
            semisolid.value()->setPositionFromBottom(trans.position);
        }
    }

    using CallbackMap = std::unordered_map<ecs::Entity, std::vector<std::pair<ecs::Entity, BoundCollisionCallback>>, ecs::EntityHash>;
    CallbackMap collisionCallbacks;

    auto checkOthersCallback = [](CallbackMap& collisionCallbacks, HitInfo hitinfo, ecs::Entity entity, IUseCollision* selfCollider) {
        IUseCollision* callbackOwnerCollider = nullptr;
        if (hitinfo.isOtherSolid) {
            callbackOwnerCollider = &hitinfo.other.get<SolidCollider>();
        } else if (hitinfo.isOtherSemiSolid) {
            callbackOwnerCollider = &hitinfo.other.get<SemiSolidCollider>();
        } else if (hitinfo.isOtherActor) {
            callbackOwnerCollider = &hitinfo.other.get<ActorCollider>();
        } else {
            return;
        }
        if (callbackOwnerCollider->getOnCollisionEnter() != nullptr) {
            addIfUnique(collisionCallbacks[hitinfo.other], hitinfo.other, entity, callbackOwnerCollider, selfCollider, hitinfo.normal);
        }
    };

    auto checkSelfCallback = [](CallbackMap& collisionCallbacks, HitInfo hitinfo, ecs::Entity entity, IUseCollision* selfCollider) {
        if (selfCollider->getOnCollisionEnter() != nullptr) {
            IUseCollision* otherCollider = nullptr;
            if (hitinfo.isOtherSolid) {
                otherCollider = &hitinfo.other.get<SolidCollider>();
            } else if (hitinfo.isOtherSemiSolid) {
                otherCollider = &hitinfo.other.get<SemiSolidCollider>();
            } else if (hitinfo.isOtherActor) {
                otherCollider = &hitinfo.other.get<ActorCollider>();
            }
            addIfUnique(collisionCallbacks[entity], entity, hitinfo.other, selfCollider, otherCollider, hitinfo.normal);
        }
    };

    for (auto& [entityid, entity] : getEntitiesRef()) {
        f32 dt;
        if (entity.has<Camera>() && !PauseMenu::instance().isPaused()) {
            // move if fake paused
            dt = System::dt.getUnmodified();
        } else {
            dt = System::dt();
        }

        std::optional<RigidBody*> rb = entity.tryGet<RigidBody>();
        Vector2f frictionMultiplier = {1, 1};
        if (rb) {
            frictionMultiplier = rb.value()->frictionMultiplier;
        }

        const f32 frictionStepGround = dt * FRICTION_GROUND * frictionMultiplier.x();
        const f32 frictionStepAir = dt * FRICTION_AIR * frictionMultiplier.y();
        const f32 gravityStep = dt * GRAVITY * 3;
        Transform2D& trans = entity.get<Transform2D>();
        Velocity& vel = entity.get<Velocity>();

        // if impulse ends, use residual
        Vector2f impulse = vel.impulse;
        if (!impulse.x() && abs(vel.residualImpulse.x()) >= MOVE_EPSILON) {
            impulse.e[0] += vel.residualImpulse.x();
        }
        if (!impulse.y() && abs(vel.residualImpulse.y()) >= MOVE_EPSILON) {
            impulse.e[1] += vel.residualImpulse.y();
        }

        const Vector2f totalVelocity = vel.stable + impulse;
        const Vector2f move = {totalVelocity.x() * dt * PIXELS_PER_TEXEL, totalVelocity.y() * dt * PIXELS_PER_TEXEL};

        vel.residualImpulse = {approach(impulse.x(), 0, frictionStepGround), approach(impulse.y(), 0, gravityStep)};
        vel.impulse = {0, 0};
        vel.total = totalVelocity;

        std::optional<ActorCollider*> actor = entity.tryGet<ActorCollider>();
        std::optional<SemiSolidCollider*> semisolid = std::nullopt;
        if (!actor) {
            semisolid = entity.tryGet<SemiSolidCollider>();
        }

        // only actors and semisolds can be grounded (because they're the only ones that interact with the ground)
        if (rb && actor) {
            // MOVEMENT + GROUNDED CHECKS
            //// no callback needed when actor moves into solid

            // Y movement
            const bool wasGrounded = rb.value()->isGrounded;
            if (auto hitinfo = actor.value()->moveY(move, nullptr, true); hitinfo) {
                if (move.y() <= 0) {
                    rb.value()->setGrounded(hitinfo.value().otherMaterial);
                } else {
                    rb.value()->setNotGrounded();
                }
                rb.value()->isJumping = false;
                vel.residualImpulse.e[1] = 0;

                checkSelfCallback(collisionCallbacks, *hitinfo, entity, *actor);
                checkOthersCallback(collisionCallbacks, *hitinfo, entity, *actor);
            } else {
                rb.value()->setNotGrounded();
            }

            // X movement
            if (auto hitinfo = actor.value()->moveX(move, nullptr); hitinfo) {
                checkSelfCallback(collisionCallbacks, *hitinfo, entity, *actor);
                checkOthersCallback(collisionCallbacks, *hitinfo, entity, *actor);
            }

            allActors.push_back(entity);

            // RIGIDBODY FLAGS, MOMENTUM, AND COYOTE TIME
            bool isMomentumStored = actor.value()->isMomentumStoredX() || actor.value()->isMomentumStoredY();
            if (rb.value()->isGrounded) {
                bool isLanding = !wasGrounded;
                rb.value()->isLanding = isLanding;
                if (isLanding) {
                    System::eventMgr.triggerEvent(Event::LANDING_EVENT, entity);
                }

                if (isMomentumStored) {
                    actor.value()->momentumNotUsed();
                }

            } else {
                if (wasGrounded && !rb.value()->isJumping) {
                    rb.value()->coyoteSecondsRemaining = rb.value()->coyoteTimeSecondsMax;
                } else if (rb.value()->coyoteSecondsRemaining > 0) {
                    rb.value()->coyoteSecondsRemaining -= dt;
                }

                // prevent repeated push forces from accumulating huge speed
                if (isMomentumStored && rb.value()->momentumCooldownFrames <= 0) {
                    // convert to texels/sec
                    vel.stable += actor.value()->getMomentum() * FTEXELS_PER_PIXEL;
                    actor.value()->resetMomentum();
                    rb.value()->momentumCooldownFrames = MOMENTUM_COOLDOWN_FRAMES;
                } else if (rb.value()->momentumCooldownFrames > 0) {
                    rb.value()->momentumCooldownFrames--;
                }
            }

        } else if (actor) {
            if (auto hitinfo = actor.value()->moveX(move, nullptr); hitinfo) {
                checkSelfCallback(collisionCallbacks, *hitinfo, entity, *actor);
                checkOthersCallback(collisionCallbacks, *hitinfo, entity, *actor);
            }
            if (auto hitinfo = actor.value()->moveY(move, nullptr); hitinfo) {
                checkSelfCallback(collisionCallbacks, *hitinfo, entity, *actor);
                checkOthersCallback(collisionCallbacks, *hitinfo, entity, *actor);
            }
            allActors.push_back(entity);

        } else if (semisolid && rb) {
            // MOVEMENT + GROUNDED CHECKS
            //// no callback needed when semisolid moves into solid

            // Y movement
            const bool wasGrounded = rb.value()->isGrounded;
            auto ridingActors = semisolid.value()->getRidingActors();
            auto ridingSemis = semisolid.value()->getRidingSemiSolids();
            if (auto hitinfo = semisolid.value()->moveY(move.y(), nullptr, ridingActors, ridingSemis, false, true); hitinfo) {
                if (move.y() <= 0) {
                    rb.value()->setGrounded(hitinfo.value().otherMaterial);
                } else {
                    rb.value()->setNotGrounded();
                }
                rb.value()->isJumping = false;
                vel.residualImpulse.e[1] = 0;

                checkSelfCallback(collisionCallbacks, *hitinfo, entity, *semisolid);
                checkOthersCallback(collisionCallbacks, *hitinfo, entity, *semisolid);
            } else {
                rb.value()->setNotGrounded();
            }

            // X movement
            if (auto hitinfo = semisolid.value()->moveX(move.x(), nullptr, ridingActors, ridingSemis); hitinfo) {
                checkSelfCallback(collisionCallbacks, *hitinfo, entity, *semisolid);
                checkOthersCallback(collisionCallbacks, *hitinfo, entity, *semisolid);
            }

            allSemiSolids.push_back(entity);

            // RESEARCH this makes me think i should separate jumping/coyote stuff into its own platformer component
            // RIGIDBODY FLAGS, AND COYOTE TIME
            if (rb.value()->isGrounded) {
                bool isLanding = !wasGrounded;
                rb.value()->isLanding = isLanding;
                if (isLanding) {
                    System::eventMgr.triggerEvent(Event::LANDING_EVENT, entity);
                }

            } else {
                if (wasGrounded && !rb.value()->isJumping) {
                    rb.value()->coyoteSecondsRemaining = rb.value()->coyoteTimeSecondsMax;
                } else if (rb.value()->coyoteSecondsRemaining > 0) {
                    rb.value()->coyoteSecondsRemaining -= dt;
                }
            }

        } else if (semisolid) {
            auto ridingActors = semisolid.value()->getRidingActors();
            auto ridingSemis = semisolid.value()->getRidingSemiSolids();

            // X movement
            auto hitinfo = semisolid.value()->moveX(move.x(), nullptr, ridingActors, ridingSemis);
            if (hitinfo) {
                checkSelfCallback(collisionCallbacks, *hitinfo, entity, *semisolid);
                checkOthersCallback(collisionCallbacks, *hitinfo, entity, *semisolid);
            }

            // Y movement
            hitinfo = semisolid.value()->moveY(move.y(), nullptr, ridingActors, ridingSemis);
            if (hitinfo) {
                checkSelfCallback(collisionCallbacks, *hitinfo, entity, *semisolid);
                checkOthersCallback(collisionCallbacks, *hitinfo, entity, *semisolid);
            }
            allSemiSolids.push_back(entity);

        } else if (std::optional<SolidCollider*> solid = entity.tryGet<SolidCollider>(); solid) {
            // TODO collision callback when we push/carry something -- i guess
            solid.value()->move(move.x(), move.y());
            trans.position = solid.value()->getCollider().getPositionEdge(Vector2i::unitDown);

        } else {
            trans.position += Vector2i(std::round(move.x()), std::round(move.y()));
        }

        // This could be made a lot faster if I have separate functions for actors/non-actors
        if (rb) {
            // friction
            if (vel.stable.x()) {
                if (rb.value()->isGrounded) {
                    applyFriction(vel.stable, frictionStepGround);
                } else {
                    applyFriction(vel.stable, frictionStepAir);
                    vel.residualImpulse.e[0] = approach(impulse.x(), 0, frictionStepAir);  // recalced
                }
            }

            // gravity
            if (!rb.value()->isGrounded) {
                if (totalVelocity.y() < JUMP_PEAK_SPEED_MAX) {
                    // falling == not jumping
                    // a little lower than 0 while applying reduced gravity
                    rb.value()->isJumping = false;
                }

                applyGravity(vel, dt, rb.value()->isJumping);

                rb.value()->isLanding = false;

            } else {
                if (totalVelocity.y() < 0 && !rb.value()->isJumping) {
                    // zero y velocity when grounded and not trying to jump, otherwise entity falls at terminal velocity after walking off platform
                    vel.stable.e[1] = 0;
                }
            }
        }
    }

    // actors/semisolids can be moved by other colliders, so wait until all collisions are processed to update position
    for (auto& entity : allActors) {
        Transform2D& trans = entity.get<Transform2D>();
        ActorCollider& actor = entity.get<ActorCollider>();

        // position is bottom-middle of collider
        trans.position = actor.getCollider().getPositionEdge(Vector2i::unitDown);
    }
    for (auto& entity : allSemiSolids) {
        Transform2D& trans = entity.get<Transform2D>();
        SemiSolidCollider& semi = entity.get<SemiSolidCollider>();

        // position is bottom-middle of collider
        trans.position = semi.getCollider().getPositionEdge(Vector2i::unitDown);
    }

    // do collision callbacks
    for (auto& [callbackEntity, hitList] : collisionCallbacks) {
        for (auto& [otherEntity, callback] : hitList) {
            callback();
        }
    }
}

}  // namespace whal
