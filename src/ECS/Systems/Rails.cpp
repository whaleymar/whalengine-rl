#include "Rails.h"

#include "ECS/Collision.h"
#include "ECS/RailsControl.h"
#include "ECS/Tags.h"
#include "ECS/Transform.h"
#include "ECS/Velocity.h"

#include "Systems/PauseMenu.h"
#include "Systems/System.h"
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
        if (entity.has<Camera>() && !PauseMenu::instance().isPaused()) {
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
            std::optional<Velocity*> vel = entity.tryGet<Velocity>();
            if (!vel || (vel.value()->stable.x() == 0 && vel.value()->stable.y() == 0)) {
                return 0;
            }
            return vel.value()->stable.len();
        }();
        f32 epsilon = speed * SPEED_DIVISOR + 1;

        if (rails.isWaiting) {
            // waiting at checkpoint
            if (rails.curTarget != 0 || rails.isCycle || rails.isVelocityUpdateNeeded) {
                if (rails.curActionTime >= rails.waitTime) {
                    // start moving
                    rails.step();
                    rails.isWaiting = false;

                    rails.isVelocityUpdateNeeded = true;

                    Vector2i newDelta = rails.getTarget().position - transform.position;

                    // prevent divide by zero
                    if (newDelta.x() == 0 && newDelta.y() == 0) {
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
            if (std::optional<Collider*> colliderOpt = entity.tryGet<Collider>(); colliderOpt) {
                colliderOpt.value()->move(delta, nullptr, false, true);
                entity.set(Transform2D(colliderOpt.value()->getShape().getPositionEdge(Vector2i::unitDown)));
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
            if (rails.isVelocityUpdateNeeded) {
                f32 speed = rails.getSpeed(transform.position);
                entity.set(Velocity(delta.norm() * speed));
            }
            rails.curActionTime += dt;
        }
    }
}

}  // namespace whal
