#include "CallbackSystem.h"

#include "Components/Callback.h"

namespace whal {

void CustomUpdateSystem::update() {
    for (auto [entityid, entity] : getEntities()) {
        // not bothering with a null check

        const auto update = entity.get<CustomUpdate>();
        update.callback(entity);
    }
}

void OnDeathSystem::onRemove(ecs::Entity entity) {
    // not bothering with a null check
    const auto onDeath = entity.get<OnDeath>();
    onDeath.callback(entity);
}

}  // namespace whal
