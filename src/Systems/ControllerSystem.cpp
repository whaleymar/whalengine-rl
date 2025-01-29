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
    for (auto& [entityid, entity] : getEntities()) {
        Velocity& vel = entity.get<Velocity>();
        PlayerControl& control = entity.get<PlayerControl>();

        f32 impulseX = 0;
        if (Input.isOn("left")) {
            impulseX -= 1;
        }
        if (Input.isOn("right")) {
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
            trans.setFacing(Facing::Right, entity);
        } else if (impulseX < 0) {
            trans.setFacing(Facing::Left, entity);
        }
    }
}

void FreeControlSystem::update() {
    for (auto& [entityid, entity] : getEntities()) {
        auto& trans = entity.get<Transform>();
        Vector2f delta;
        if (Input.isOn("left")) {
            delta += Vector2f::LEFT;
            trans.setFacing(Facing::Left, entity);
        }
        if (Input.isOn("right")) {
            delta += Vector2f::RIGHT;
            trans.setFacing(Facing::Right, entity);
        }
        if (Input.isOn("up")) {
            delta += Vector2f::UP;
        }
        if (Input.isOn("down")) {
            delta += Vector2f::DOWN;
        }

        if (!delta.isZero()) {
            delta = delta.norm();  // diagonal speed should match speed in cardinal directions
        }

        auto control = entity.get<PlayerControl>();
        delta *= control.moveSpeed;

        entity.get<Velocity>().stable = delta;
    }
}

void JumpSystem::update() {
    const bool isJumpPressedThisFrame = Input.isPressed("jump");

    for (auto& [entityid, entity] : getEntities()) {
        Velocity& vel = entity.get<Velocity>();
        Jumper& jumpControl = entity.get<Jumper>();
        RigidBody& rb = entity.get<RigidBody>();

        f32 impulseY = 0;
        if (isJumpPressedThisFrame) {
            jumpControl.buffer.buffer();
        }

        // || entity.has<AIControl>() && AIControl.isJumping()
        if (entity.has<PlayerControl>() && Input.isHeld("jump")) {
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
                particleBurst(entity.get<Transform>(), direction, rb.groundMaterial, 8, Depth::Foreground3, 0.5, 2.5, 10);

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
