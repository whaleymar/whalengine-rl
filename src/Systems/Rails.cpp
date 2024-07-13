#include "Rails.h"

#include "Components/Collision.h"
#include "Components/RailsControl.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Components/Velocity.h"

#include "Sys/PauseMenu.h"
#include "Sys/System.h"
#include "Util/Print.h"
#include "Util/Vector.h"

namespace whal {

constexpr f32 SPEED_DIVISOR = 1.0f / 40.0f;

void RailsSystem::onAdd(const ecs::Entity entity) {
    auto& trans = entity.get<Transform2D>();
    auto& rails = entity.get<RailsControl>();
    rails.prepareForFirstStep(trans);
}

void RailsSystem::update() {
    for (auto& [entityid, entity] : getEntitiesRef()) {
        f32 dt;
        // camera moves normally unless pause menu is activated
        if (entity.has<Camera>() && !PauseMenu::instance().isActive()) {
            dt = System::dt.getUnmodified();
        } else {
            dt = System::dt();
        }
        auto& rails = entity.get<RailsControl>();
        if (!rails.isValid()) {
            print("skipping invalid RailsControl component for entity", entityid);
            continue;
        }
        auto& transform = entity.get<Transform2D>();

        const Vector2f delta = toFloatVec(rails.getTarget().position - transform.position);
        f32 distance = delta.len();

        // scale checkpoint threshold with speed
        f32 speed = [entity]() -> f32 {
            auto velOpt = entity.tryGet<Velocity>();
            if (!velOpt || ((*velOpt)->stable.x() == 0 && (*velOpt)->stable.y() == 0)) {
                return 0;
            }
            return (*velOpt)->stable.len();
        }();
        f32 epsilon = speed >= 50 ? speed * SPEED_DIVISOR + 1 : 0.95;

        if (rails.isWaiting) {
            // waiting at checkpoint
            if (rails.isNextStepAutomatic() || rails.isVelocityUpdateNeeded) {
                if (rails.curActionTime >= rails.waitTime) {
                    // start moving
                    rails.step();
                    rails.isWaiting = false;

                    rails.isVelocityUpdateNeeded = true;

                    Vector2i newDelta = rails.getTarget().position - transform.position;

                    // prevent divide by zero
                    if (newDelta.isZero()) {
                        // we're already at target for some reason, so no need to wait again
                        entity.add<Velocity>();
                        rails.curActionTime = rails.waitTime;
                    } else {
                        Velocity velToAdd = Velocity(toFloatVec(newDelta).norm() * rails.getSpeed(transform.position));
                        entity.add<Velocity>(velToAdd);
                    }

                } else {
                    rails.curActionTime += dt;
                }
            }

        } else if (distance <= epsilon) {
            // got to checkpoint, clamp to exact position

            // if entity has collider, use its move function
            if (auto colliderOpt = entity.tryGet<Collider>(); colliderOpt) {
                (*colliderOpt)->move(delta, nullptr, false, true);
            } else {
                entity.set(Transform2D(rails.getTarget().position));
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
                f32 speed = rails.getSpeed(transform.position);
                entity.set(Velocity(delta.norm() * speed));
            }
            rails.curActionTime += dt;
        }
    }
}

}  // namespace whal
