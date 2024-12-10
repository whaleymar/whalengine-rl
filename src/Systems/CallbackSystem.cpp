#include "CallbackSystem.h"

#include "Components/Callback.h"

namespace whal {

void OnFrameEndSystem::update() {
    for (auto [entityid, entity] : getEntitiesCopy()) {
        // not bothering with a null check

        const auto onFrameEnd = entity.get<OnFrameEnd>();
        onFrameEnd.callback(entity);

        if (onFrameEnd.removeSelf) {
            entity.remove<OnFrameEnd>();
        }
    }
}

void CustomUpdateSystem::update() {
    for (auto [entityid, entity] : getEntities()) {
        // not bothering with a null check

        const auto onFrameEnd = entity.get<CustomUpdate>();
        onFrameEnd.callback(entity);
    }
}

void OnDeathSystem::onRemove(ecs::Entity entity) {
    // not bothering with a null check
    const auto onDeath = entity.get<OnDeath>();
    onDeath.callback(entity);
}

}  // namespace whal
