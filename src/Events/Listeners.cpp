#include "Listeners.h"

#include "Sys/System.h"

namespace whal {

// ECS callback
void emitEntityDeathEvent(ecs::Entity entity) {
    Event.emit<evt::Death>(entity);
}

}  // namespace whal
