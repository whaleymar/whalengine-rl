#include "CallbackSystem.h"

#include "Components/Callback.h"
#include "Sys/System.h"

namespace whal {

void CustomUpdateSystem::update() {
    for (auto [entityid, entity] : getEntities()) {
        // not bothering with a null check

        const auto update = entity.get<CustomUpdate>();
        if (Time.getFrame() % update.everyNFrame == 0) {
            update.callback(entity);
        }
    }
}

void OnDeathSystem::onRemove(ecs::Entity entity) {
    // not bothering with a null check
    const auto onDeath = entity.get<OnDeath>();
    onDeath.callback(entity);
}

}  // namespace whal
