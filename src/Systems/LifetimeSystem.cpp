#include "LifetimeSystem.h"

#include "Components/Callback.h"
#include "Components/Lifetime.h"
#include "Sys/System.h"
#include "Sys/Time.h"

namespace whal {

void LifetimeSystem::update() {
    f32 dt = Time.dt();
    for (auto [entityid, entity] : getEntities()) {
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

}  // namespace whal
