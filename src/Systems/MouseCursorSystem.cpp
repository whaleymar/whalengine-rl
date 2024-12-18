#include "MouseCursorSystem.h"

#include "Sys/System.h"

namespace whal {

void MouseCursorSystem::update() {
    for (auto [id, entity] : getEntities()) {
        Vector2i position = Input.getMouseWorld();

        // update transform
        entity.get<Transform>().setPosition(position.as<f32>(), entity);
    }
}

}  // namespace whal
