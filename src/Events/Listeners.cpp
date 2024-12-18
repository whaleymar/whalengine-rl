#include "Listeners.h"

#include "Components/Transform.h"
#include "Sys/System.h"

namespace whal {

// ECS callback
void emitEntityDeathEvent(ecs::Entity entity) {
    Event.emit<evt::Death>(entity);
}

void onTopLevelEntityCreated(ecs::Entity entity) {
    entity.add<Transform>();
}

void onChildEntityCreated(ecs::Entity child, ecs::Entity parent) {
    Transform trans;
    trans.setParent(parent.get<Transform>(), child);
    child.add(trans);
}

void onEntityAdopted(ecs::Entity child, ecs::Entity parent) {
    child.get<Transform>().setParent(parent.get<Transform>(), child);
}

}  // namespace whal
