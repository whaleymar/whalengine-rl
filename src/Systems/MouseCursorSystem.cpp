#include "MouseCursorSystem.h"

#include "Components/Tags.h"
#include "Sys/InputHandler.h"
#include "Sys/System.h"

namespace whal {

void MouseCursorSystem::update() {
    if (System::isEnginePaused()) {
        return;
    }
    for (auto [id, entity] : getEntities()) {
        // update transform
        entity.get<Transform>().setPositionManually(Input.getMouseWorld(), entity);

        // hack: make cursor invisible if gamepad is enabled
        if (Input.getIsGamepadAllowed() && !entity.has<Invisible>()) {
            entity.add<Invisible>();
        } else if (!Input.getIsGamepadAllowed() && entity.has<Invisible>()) {
            entity.remove<Invisible>();
        }
    }
}

}  // namespace whal
