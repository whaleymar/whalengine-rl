#include "Lifetime.h"

#include "Components/Callback.h"
#include "Components/Draw.h"
#include "Components/Lifetime.h"
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

// void SlowEntityKillerSystem::update() {
//     for (auto [entityid, entity] : getEntitiesMutable()) {
//         const auto speedBelowComponent = entity.get<DieWhenSpeedBelow>();
//         const f32 minSpeed = speedBelowComponent.minSpeed;
//         const f32 speed = entity.get<Velocity>().total.len();
//         if (speed <= minSpeed) {
//             if (!speedBelowComponent.colorFade.isDone()) {
//                 entity.add(Lifetime(speedBelowComponent.colorFade.duration));
//                 entity.add(speedBelowComponent.colorFade);
//             }
//             entity.add(OnFrameEnd([](ecs::Entity e) { e.remove<DieWhenSpeedBelow>(); }));
//         }
//     }
// }

}  // namespace whal
