#include "Listeners.h"

#include "Systems/System.h"

namespace whal {

// ECS callback
void emitEntityDeathEvent(ecs::Entity entity) {
    System::eventMgr.triggerEvent(Event::DEATH, entity);
}

}  // namespace whal
