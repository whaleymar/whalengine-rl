#include "Listeners.h"

#include "Sys/System.h"

namespace whal {

// ECS callback
void emitEntityDeathEvent(ecs::Entity entity) {
    System::event.emit<DeathEvent>(entity);
}

}  // namespace whal
