#include "Listeners.h"

#include "Sys/System.h"

namespace whal {

// ECS callback
void emitEntityDeathEvent(ecs::Entity entity) {
    System::eventMgr.triggerEvent<DeathEvent>(entity);
}

}  // namespace whal
