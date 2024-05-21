#include "Controller.h"

#include "ECS/Draw.h"
#include "ECS/PlayerControl.h"
#include "ECS/RigidBody.h"
#include "ECS/Transform.h"
#include "ECS/Velocity.h"

#include "Systems/System.h"
#include "Util/MathUtil.h"

namespace whal {

constexpr f32 APPROACH_SPEED_X = 7.5;  // 5 frames to max speed

void ControllerSystemRB::update() {
    auto& input = System::input;
    f32 dt = System::dt();

    bool isJumpAvailable = input.isJumpAvailable();
    input.useJump();

    for (auto& [entityid, entity] : getEntitiesRef()) {
        Velocity& vel = entity.get<Velocity>();
        PlayerControlRB& control = entity.get<PlayerControlRB>();

#ifndef NDEBUG
        auto sprite = entity.tryGet<Sprite>();
        if (sprite) {
            bool changed = false;
            if (input.isShrinkX()) {
                sprite.value()->scale.e[0] -= 0.1;
                changed = true;
            }
            if (input.isShrinkY()) {
                sprite.value()->scale.e[1] -= 0.1;
                changed = true;
            }
            if (input.isGrowX()) {
                sprite.value()->scale.e[0] += 0.1;
                changed = true;
            }
            if (input.isGrowY()) {
                sprite.value()->scale.e[1] += 0.1;
                changed = true;
            }
            if (changed) {
                sprite.value()->isVertsUpdateNeeded = true;
            }
        }
#endif

        f32 impulseX = 0;
        if (input.isLeft()) {
            impulseX -= 1;
        }
        if (input.isRight()) {
            impulseX += 1;
        }

        // controls can only speed us up, not slow us down (assuming trying to move the same direction as current velocity)
        impulseX *= control.moveSpeed;
        const f32 approachSpeed = APPROACH_SPEED_X * dt * control.moveSpeed;
        if (impulseX != 0) {
            f32 approachFrom;
            approachFrom = vel.stable.x();
            if (sign(impulseX) == sign(vel.stable.x())) {
                if (abs(approachFrom) < control.moveSpeed) {
                    // approach max move speed
                    impulseX = approach(approachFrom, impulseX, approachSpeed);
                    vel.stable.e[0] = impulseX;
                }
            } else {
                approachFrom = 0;
                impulseX = approach(approachFrom, impulseX, approachSpeed);
                vel.stable.e[0] += impulseX;
            }
        }

        // Y axis controls (jumping) works different because this feels better
        f32 impulseY = 0;
        if (isJumpAvailable) {
            control.jumpBuffer.buffer();
        }

        RigidBody& rb = entity.get<RigidBody>();
        if (input.isJump()) {
            if ((rb.isGrounded || rb.coyoteSecondsRemaining > 0) && control.canJump()) {
                control.jumpBuffer.consume();
                rb.isJumping = true;

                if (vel.stable.y() < 0) {
                    vel.stable.e[1] = 0;
                }

                impulseY += rb.jumpInitialVelocity;
                rb.jumpSecondsRemaining = rb.jumpSecondsMax;
            } else if (control.isJumping() && rb.isJumping) {
                f32 damping = rb.jumpSecondsRemaining / rb.jumpSecondsMax;
                damping *= damping;
                impulseY += rb.jumpInitialVelocity * damping;
                rb.jumpSecondsRemaining -= dt;
                if (rb.jumpSecondsRemaining < 0) {
                    rb.jumpSecondsRemaining = 0;
                    control.jumpBuffer.reset();
                    rb.isJumping = false;
                }
            } else if (control.canJump()) {
                control.jumpBuffer.notUsed();
            }
        } else {
            control.jumpBuffer.reset();
            rb.isJumping = false;
        }

        vel.impulse += Vector2f(0.0, impulseY);

        auto& trans = entity.get<Transform2D>();
        if (impulseX > 0 && trans.facing != Facing::Right) {
            trans.facing = Facing::Right;
            trans.isDirectionChanged = true;
        } else if (impulseX < 0 && trans.facing != Facing::Left) {
            trans.facing = Facing::Left;
            trans.isDirectionChanged = true;
        }
    }
}

void ControllerSystemFree::update() {
    auto& input = System::input;
    for (auto& [entityid, entity] : getEntitiesRef()) {
        Vector2f delta;
        if (input.isLeft()) {
            delta += Vector2f::unitLeft;
        }
        if (input.isRight()) {
            delta += Vector2f::unitRight;
        }
        if (input.isUp()) {
            delta += Vector2f::unitUp;
        }
        if (input.isDown()) {
            delta += Vector2f::unitDown;
        }

        PlayerControlFree& control = entity.get<PlayerControlFree>();
        delta *= control.moveSpeed;

        Velocity newVel = Velocity(delta);
        entity.set(newVel);
    }
}

}  // namespace whal
