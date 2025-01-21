#include "RailsSystem.h"

#include "Components/Collision.h"
#include "Components/RailsControl.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Components/Velocity.h"

#include "Sys/System.h"
#include "Sys/Tween.h"
#include "Util/Print.h"
#include "Util/Vector.h"

namespace whal {

constexpr f32 SPEED_DIVISOR = 1.0f / 40.0f;

void RailsSystem::onAdd(const ecs::Entity entity) {
    auto& trans = entity.get<Transform>();
    auto& rails = entity.get<RailsControl>();
    rails.prepareForFirstStep(trans, entity);
    if (entity.has<Velocity>()) {
        rails.isPhysicsEntity = true;
    } else {
        rails.isPhysicsEntity = false;
    }
}

static void updatePhysicsRails(ecs::Entity entity, RailsControl& rails) {
    f32 dt;
    if (entity.has<IgnoreTimeModifiers>()) {
        dt = Time.getUnmodified();
    } else {
        dt = Time.dt();
    }
    auto& transform = entity.get<Transform>();

    const Vector2f delta = (rails.getTarget().position - transform.positionPx).as<f32>();
    f32 distance = delta.len();

    // scale checkpoint threshold with speed
    f32 entitySpeed = [entity]() -> f32 {
        auto velOpt = entity.tryGet<Velocity>();
        if (!velOpt || (velOpt->stable.x == 0 && velOpt->stable.y == 0)) {
            return 0;
        }
        return velOpt->stable.len();
    }();
    f32 epsilon = entitySpeed >= 50 ? entitySpeed * SPEED_DIVISOR + 1 : 0.95;

    if (rails.isWaiting) {
        // waiting at checkpoint
        if (rails.isNextStepAutomatic() || rails.isVelocityUpdateNeeded) {
            if (rails.curActionTime >= rails.waitTime) {
                // start moving
                rails.step();
                rails.isWaiting = false;

                rails.isVelocityUpdateNeeded = true;

                Vector2i newDelta = rails.getTarget().position - transform.positionPx;

                // prevent divide by zero
                if (newDelta.isZero()) {
                    // we're already at target for some reason, so no need to wait again
                    entity.add<Velocity>();
                    rails.curActionTime = rails.waitTime;
                } else {
                    Velocity velToAdd = Velocity::from(newDelta.as<f32>().norm() * rails.getSpeed(transform.positionPx));
                    entity.add<Velocity>(velToAdd);
                }

            } else {
                rails.curActionTime += dt;
            }
        }

    } else if (distance <= epsilon) {
        // got to checkpoint, clamp to exact position

        if (entity.has<Collider>()) {
            entity.get<Collider>().move(delta, nullptr, false, true, false, false, true);
        } else {
            entity.set(Transform::world(rails.getTarget().position));
        }

        entity.remove<Velocity>();
        rails.isWaiting = true;
        rails.curActionTime = 0;
        rails.isVelocityUpdateNeeded = false;
        if (rails.arrivalCallback != nullptr) {
            rails.arrivalCallback(entity, rails);
        }

    } else {
        // moving to next checkpoint
        if (rails.isVelocityUpdateNeeded && !delta.isZero()) {
            f32 speed = rails.getSpeed(transform.positionPx);
            entity.set(Velocity::from(delta.norm() * speed));
        }
        rails.curActionTime += dt;
    }
}

static void updateTweenRails(ecs::Entity entity, RailsControl& rails) {
    f32 dt;
    // camera moves normally unless pause menu is activated
    if (entity.has<IgnoreTimeModifiers>()) {
        dt = Time.getUnmodified();
    } else {
        dt = Time.dt();
    }
    if (rails.isWaiting) {
        // waiting at checkpoint
        if (rails.isNextStepAutomatic() || rails.isVelocityUpdateNeeded) {
            if (rails.curActionTime >= rails.waitTime) {
                // start moving
                rails.step();
                rails.isWaiting = false;
                rails.isVelocityUpdateNeeded = true;

                // add tween
                const Vector2f targetPosF = rails.getTarget().position.as<f32>();
                const f32 segmentDistance = (targetPosF - rails.startPosition).len();
                const f32 time = segmentDistance / rails.speed;

                Schedule.tween(entity, rails.getTarget().position.as<f32>(), time, &Transform::position, &Transform::setPosition)
                    .setTransition(rails.getTarget().movement)
                    .setOnEnd([](ecs::Entity entity, const Tween<Vector2f>&) {
                        auto& rails = entity.get<RailsControl>();
                        rails.isWaiting = true;
                        rails.curActionTime = 0;
                        rails.isVelocityUpdateNeeded = false;
                        if (rails.arrivalCallback != nullptr) {
                            rails.arrivalCallback(entity, rails);
                        }
                    });

            } else {
                rails.curActionTime += dt;
            }
        }

    } else {
        // moving to next checkpoint
        rails.curActionTime += dt;
    }
}

void RailsSystem::update() {
    for (auto& [entityid, entity] : getEntities()) {
        auto& rails = entity.get<RailsControl>();
        if (!rails.isValid()) {
            print("skipping invalid RailsControl component for entity", entity.id());
            continue;
        }
        if (rails.isPhysicsEntity) {
            updatePhysicsRails(entity, rails);
        } else {
            updateTweenRails(entity, rails);
        }
    }
}

}  // namespace whal
