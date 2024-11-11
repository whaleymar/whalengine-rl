#include "LifetimeSystem.h"

#include "Components/Callback.h"
#include "Components/Lifetime.h"
#include "Sys/System.h"

namespace whal {

void LifetimeSystem::update() {
    f32 dt = Time.dt();
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

}  // namespace whal
