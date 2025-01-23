#include "CallbackSystem.h"

#include "Components/Callback.h"
#include "Sys/System.h"

namespace whal {

void CustomUpdateSystem::update() {
#ifndef NDEBUG
    if (System::isEnginePaused()) {
        return;
    }
#endif
    for (auto [entityid, entity] : getEntities()) {
        const auto update = entity.get<CustomUpdate>();
        assert(update.callback != nullptr && "CustomUpdate::update is null!");
        if (Time.getFrame() % update.everyNFrame == 0) {
            update.callback(entity);
        }
    }
}

void OnDeathSystem::onRemove(ecs::Entity entity) {
    const auto onDeath = entity.get<OnDeath>();
    assert(onDeath.callback != nullptr && "CustomUpdate::update is null!");
    onDeath.callback(entity);
}

}  // namespace whal
