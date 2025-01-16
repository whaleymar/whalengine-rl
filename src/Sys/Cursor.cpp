#include "Cursor.h"

#include <raylib.h>
#include "Components/Draw.h"
#include "Components/Name.h"
#include "Components/Tags.h"
#include "Gfx/Depth.h"
#include "Sys/System.h"
#include "Systems/MouseCursorSystem.h"

namespace whal {

void CursorManager::set(Sprite sprite) const {
    const bool isCustomCursorActive = !MouseCursorSystem::getEntities().empty();
    if (!isCustomCursorActive) {
        auto entity = World.entity();
        entity.add(Name("Cursor"));
        entity.get<Transform>().depth = Depth::UIClose;
        entity.add(sprite);
        entity.add<MouseCursor>();
    } else {
        auto entity = MouseCursorSystem::first();
        entity.remove<Sprite>();
        entity.add(sprite);
    }

#ifndef NDEBUG
    if (!EDITOR_MODE) {
        rl::HideCursor();
    }
#endif
}

void CursorManager::setDefault() const {
    const bool isCustomCursorActive = !MouseCursorSystem::getEntities().empty();
    if (isCustomCursorActive) {
        MouseCursorSystem::first().kill();
    }
    rl::ShowCursor();
}

}  // namespace whal
