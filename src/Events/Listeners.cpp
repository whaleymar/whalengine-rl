#include "Listeners.h"

#include "Systems/System.h"

namespace whal {

// ECS callback
void emitEntityDeathEvent(ecs::Entity entity) {
    System::eventMgr.triggerEvent(Event::DEATH_EVENT, entity);
}

}  // namespace whal
