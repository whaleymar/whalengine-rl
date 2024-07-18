#include "Lifetime.h"

#include "Components/Callback.h"
#include "Components/Draw.h"
#include "Components/Lifetime.h"
#include "Components/Velocity.h"
#include "Sys/System.h"

namespace whal {

void LifetimeSystem::update() {
    f32 dt = System::dt();
    for (auto [entityid, entity] : getEntitiesMutable()) {
        auto& lifetime = entity.get<Lifetime>();
        lifetime.secondsRemaining -= dt;
        if (lifetime.secondsRemaining <= 0) {
            if (lifetime.onDeath != nullptr) {
                lifetime.onDeath(entity);
            }
            entity.kill();
        }
    }
}

void SlowEntityKillerSystem::update() {
    for (auto [entityid, entity] : getEntitiesMutable()) {
        const auto yeah = entity.get<DieWhenSpeedBelow>();
        const f32 minSpeed = yeah.minSpeed;
        const f32 speed = entity.get<Velocity>().total.len();
        if (speed < minSpeed) {
            if (yeah.delaySeconds > 0.0f) {
                entity.add(Lifetime(yeah.delaySeconds));
                if (yeah.fadeOut) {
                    entity.add(FadeOut(yeah.delaySeconds));
                }
            }
            entity.add(OnFrameEnd([](ecs::Entity e) { e.remove<DieWhenSpeedBelow>(); }));
        }
    }
}

}  // namespace whal
