#include "ControllerSystem.h"

#include "Components/PlayerControl.h"
#include "Components/RigidBody.h"
#include "Components/Transform.h"
#include "Components/Velocity.h"

#include "Entities/Particle.h"
#include "Sys/InputHandler.h"
#include "Sys/System.h"
#include "Util/MathUtil.h"

namespace whal {

constexpr f32 APPROACH_SPEED_X = 7.5;  // 5 frames to max speed

void ControllerSystem::update() {
    if (!Input.isMovementEnabled()) {
        return;
    }

    for (auto& [entityid, entity] : getEntitiesMutable()) {
        Velocity& vel = entity.get<Velocity>();
        PlayerControl& control = entity.get<PlayerControl>();

        f32 impulseX = 0;
        if (Input.isOn(InputType::LEFT)) {
            impulseX -= 1;
        }
        if (Input.isOn(InputType::RIGHT)) {
            impulseX += 1;
        }

        // controls can only speed us up, not slow us down (assuming trying to move the same direction as current velocity)
        impulseX *= control.moveSpeed;
        const f32 approachSpeed = APPROACH_SPEED_X * Time.dt() * control.moveSpeed;
        if (impulseX != 0) {
            f32 approachFrom;
            approachFrom = vel.stable.x;
            if (math::sign(impulseX) == math::sign(vel.stable.x)) {
                if (math::abs(approachFrom) < control.moveSpeed) {
                    // approach max move speed
                    impulseX = math::approach(approachFrom, impulseX, approachSpeed);
                    vel.stable.x = impulseX;
                }
            } else {
                approachFrom = 0;
                impulseX = math::approach(approachFrom, impulseX, approachSpeed);
                vel.stable.x += impulseX;
            }
        }

        auto& trans = entity.get<Transform>();
        if (impulseX > 0) {
            trans.facing = Facing::Right;
        } else if (impulseX < 0) {
            trans.facing = Facing::Left;
        }
    }
}

void FreeControlSystem::update() {
    if (!Input.isMovementEnabled()) {
        return;
    }

    for (auto& [entityid, entity] : getEntitiesMutable()) {
        auto& trans = entity.get<Transform>();
        Vector2f delta;
        if (Input.isOn(InputType::LEFT)) {
            delta += Vector2f::LEFT;
            trans.facing = Facing::Left;
        }
        if (Input.isOn(InputType::RIGHT)) {
            delta += Vector2f::RIGHT;
            trans.facing = Facing::Right;
        }
        if (Input.isOn(InputType::UP)) {
            delta += Vector2f::UP;
        }
        if (Input.isOn(InputType::DOWN)) {
            delta += Vector2f::DOWN;
        }

        if (!delta.isZero()) {
            delta = delta.norm();  // diagonal speed should match speed in cardinal directions
        }

        auto control = entity.get<PlayerControl>();
        delta *= control.moveSpeed;

        Velocity newVel = Velocity(delta);
        entity.set(newVel);
    }
}

void JumpSystem::update() {
    if (!Input.isJumpingEnabled()) {
        return;
    }

    bool isJumpPressedThisFrame = Input.isJumpAvailable();
    Input.useJump();

    for (auto& [entityid, entity] : getEntitiesMutable()) {
        Velocity& vel = entity.get<Velocity>();
        Jumper& jumpControl = entity.get<Jumper>();
        RigidBody& rb = entity.get<RigidBody>();

        f32 impulseY = 0;
        if (isJumpPressedThisFrame) {
            jumpControl.buffer.buffer();
        }

        // || entity.has<AIControl>() && AIControl.isJumping()
        if (entity.has<PlayerControl>() && Input.isOn(InputType::JUMP)) {
            if ((rb.isGrounded || jumpControl.coyoteSecondsRemaining > 0) && jumpControl.canJump()) {
                // jump happens
                jumpControl.buffer.consume();
                jumpControl.isJumping = true;

                if (vel.stable.y < 0) {
                    vel.stable.y = 0;
                }

                impulseY += jumpControl.jumpInitialVelocity;
                jumpControl.jumpSecondsRemaining = jumpControl.jumpSecondsMax;

                Direction direction = Direction::N;
                if (vel.total.x > 0) {
                    direction = Direction::NE;
                } else if (vel.total.x < 0) {
                    direction = Direction::NW;
                }
                particleBurst(entity.get<Transform>(), direction, rb.groundMaterial, 8, Depth::Foreground3, 0.5, 0.5);

            } else if (jumpControl.isTryingJump() && jumpControl.isJumping) {
                // jump button pressed and entity still in jump state
                f32 damping = jumpControl.jumpSecondsRemaining / jumpControl.jumpSecondsMax;
                damping *= damping;
                impulseY += jumpControl.jumpInitialVelocity * damping;
                jumpControl.jumpSecondsRemaining -= Time.dt();
                if (jumpControl.jumpSecondsRemaining < 0) {
                    jumpControl.jumpSecondsRemaining = 0;
                    jumpControl.buffer.reset();
                    jumpControl.isJumping = false;
                }
            } else if (jumpControl.canJump()) {
                // can jump, but didn't
                jumpControl.buffer.notUsed();
            }
        } else {
            jumpControl.buffer.reset();
            jumpControl.isJumping = false;
        }

        vel.impulse += Vector2f(0.0, impulseY);
    }
}

}  // namespace whal
