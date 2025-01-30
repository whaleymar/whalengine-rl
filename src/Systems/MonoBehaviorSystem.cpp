#include "MonoBehaviorSystem.h"

#include "Components/MonoBehavior.h"

namespace whal {

// RESEARCH: track which methods an entity overrides, and only call event functions for those entities?
// e.g., if there are 5000 MonoBehavior entities but only 2 of them use onInput, only call it for those 2
// I have no idea how to implement that

void MonoBehaviorSystem::update() {
    for (auto [entityid, entity] : getEntities()) {
        MonoBehavior& mono = entity.get<MonoBehavior>();
        mono.pBehavior->update(entity);
    }
}

void MonoBehaviorSystem::onAdd(ecs::Entity entity) {
    entity.get<MonoBehavior>().pBehavior->start(entity);
}

void MonoBehaviorSystem::onRemove(ecs::Entity entity) {
    entity.get<MonoBehavior>().pBehavior->onDestroy(entity);
}

void MonoBehaviorSystem::onEvent(evt::Input, InputEvent input) {
    for (auto [entityid, entity] : getEntities()) {
        MonoBehavior& mono = entity.get<MonoBehavior>();
        mono.pBehavior->onInput(entity, input);
    }
}

}  // namespace whal
