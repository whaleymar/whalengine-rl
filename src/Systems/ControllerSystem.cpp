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
    if (!System::input.isMovementEnabled()) {
        return;
    }

    for (auto& [entityid, entity] : getEntitiesMutable()) {
        Velocity& vel = entity.get<Velocity>();
        PlayerControl& control = entity.get<PlayerControl>();

        f32 impulseX = 0;
        if (System::input.isOn(InputType::LEFT)) {
            impulseX -= 1;
        }
        if (System::input.isOn(InputType::RIGHT)) {
            impulseX += 1;
        }

        // controls can only speed us up, not slow us down (assuming trying to move the same direction as current velocity)
        impulseX *= control.moveSpeed;
        const f32 approachSpeed = APPROACH_SPEED_X * System::dt() * control.moveSpeed;
        if (impulseX != 0) {
            f32 approachFrom;
            approachFrom = vel.stable.x;
            if (sign(impulseX) == sign(vel.stable.x)) {
                if (abs(approachFrom) < control.moveSpeed) {
                    // approach max move speed
                    impulseX = approach(approachFrom, impulseX, approachSpeed);
                    vel.stable.x = impulseX;
                }
            } else {
                approachFrom = 0;
                impulseX = approach(approachFrom, impulseX, approachSpeed);
                vel.stable.x += impulseX;
            }
        }

        auto& trans = entity.get<Transform2D>();
        if (impulseX > 0) {
            trans.facing = Facing::Right;
        } else if (impulseX < 0) {
            trans.facing = Facing::Left;
        }
    }
}

void FreeControlSystem::update() {
    if (!System::input.isMovementEnabled()) {
        return;
    }

    for (auto& [entityid, entity] : getEntitiesMutable()) {
        auto& trans = entity.get<Transform2D>();
        Vector2f delta;
        if (System::input.isOn(InputType::LEFT)) {
            delta += Vector2f::unitLeft;
            trans.facing = Facing::Left;
        }
        if (System::input.isOn(InputType::RIGHT)) {
            delta += Vector2f::unitRight;
            trans.facing = Facing::Right;
        }
        if (System::input.isOn(InputType::UP)) {
            delta += Vector2f::unitUp;
        }
        if (System::input.isOn(InputType::DOWN)) {
            delta += Vector2f::unitDown;
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
    if (!System::input.isJumpingEnabled()) {
        return;
    }

    bool isJumpPressedThisFrame = System::input.isJumpAvailable();
    System::input.useJump();

    for (auto& [entityid, entity] : getEntitiesMutable()) {
        Velocity& vel = entity.get<Velocity>();
        Jumper& jumpControl = entity.get<Jumper>();
        RigidBody& rb = entity.get<RigidBody>();

        f32 impulseY = 0;
        if (isJumpPressedThisFrame) {
            jumpControl.buffer.buffer();
        }

        // || entity.has<AIControl>() && AIControl.isJumping()
        if (entity.has<PlayerControl>() && System::input.isOn(InputType::JUMP)) {
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
                particleBurst(entity.get<Transform2D>(), direction, rb.groundMaterial, 8, Depth::Foreground3, 0.5, 0.5);

            } else if (jumpControl.isTryingJump() && jumpControl.isJumping) {
                // jump button pressed and entity still in jump state
                f32 damping = jumpControl.jumpSecondsRemaining / jumpControl.jumpSecondsMax;
                damping *= damping;
                impulseY += jumpControl.jumpInitialVelocity * damping;
                jumpControl.jumpSecondsRemaining -= System::dt();
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
