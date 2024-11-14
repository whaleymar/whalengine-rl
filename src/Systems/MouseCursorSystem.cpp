#include "MouseCursorSystem.h"

#include "Sys/System.h"

namespace whal {

void MouseCursorSystem::update() {
    for (auto [id, entity] : getEntitiesMutable()) {
        Vector2i position = Input.getMouseWorld();

        // update transform
        entity.get<Transform2D>().position = position;
    }
}

}  // namespace whal
