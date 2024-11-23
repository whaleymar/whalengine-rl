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
    const bool isCustomCursorActive = !MouseCursorSystem::getEntitiesMutable().empty();
    if (!isCustomCursorActive) {
        auto entity = World.entity().value();
        entity.add(Name("Cursor"));
        Transform2D trans;
        trans.depth = Depth::UIClose;
        entity.add(trans);
        entity.add(sprite);
        entity.add<MouseCursor>();
    } else {
        auto entity = MouseCursorSystem::first();
        entity.remove<Sprite>();
        entity.add(sprite);
    }
    rl::HideCursor();
}

void CursorManager::setDefault() const {
    const bool isCustomCursorActive = !MouseCursorSystem::getEntitiesMutable().empty();
    if (isCustomCursorActive) {
        MouseCursorSystem::first().kill();
    }
    rl::ShowCursor();
}

}  // namespace whal
