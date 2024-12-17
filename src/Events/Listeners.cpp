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

// TODO change component names for these 2 functions
void onChildEntityCreated(ecs::Entity child, ecs::Entity parent) {
    UltimateTransformFinal trans;
    trans.setParent(parent.get<UltimateTransformFinal>(), child);
    child.add(trans);
}

void onOrphanEntityAdopted(ecs::Entity child, ecs::Entity parent) {
    child.get<UltimateTransformFinal>().setParent(parent.get<UltimateTransformFinal>(), child);
}

}  // namespace whal
