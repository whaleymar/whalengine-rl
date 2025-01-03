#include "MouseCursorSystem.h"

#include "Sys/System.h"

namespace whal {

void MouseCursorSystem::update() {
    for (auto [id, entity] : getEntities()) {
        // update transform
        entity.get<Transform>().setPosition(Input.getMouseWorld(), entity);
    }
}

}  // namespace whal
