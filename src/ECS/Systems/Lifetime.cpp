#include "Lifetime.h"

#include "ECS/Lifetime.h"
#include "Systems/System.h"

namespace whal {

void LifetimeSystem::fixedUpdate() {
    f32 dt = System::dt();
    for (auto [entityid, entity] : getEntitiesRef()) {
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
