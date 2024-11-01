#include "EngineUtil.h"

#include <raylib.h>
#include "Components/Callback.h"
#include "Components/Tags.h"
#include "Gfx/Coordinates.h"
#include "Sys/System.h"
#include "Systems/MouseCursorSystem.h"
#include "whalECS/src/ECS.h"

#include "Components/Name.h"

namespace whal {

void setCustomCursor(Sprite drawComponent) {
    const bool isCustomCursorActive = !MouseCursorSystem::getEntitiesMutable().empty();
    if (!isCustomCursorActive) {
        auto entity = System::world.entity().value();
        entity.add(Name("Cursor"));
        Transform2D trans;
        trans.depth = Depth::UIClose;
        entity.add(trans);
        entity.add(drawComponent);
        entity.add<MouseCursor>();
    } else {
        auto entity = MouseCursorSystem::first();
        entity.remove<Sprite>();
        entity.add(drawComponent);
    }
    HideCursor();
}

void setDefaultCursor() {
    const bool isCustomCursorActive = !MouseCursorSystem::getEntitiesMutable().empty();
    if (isCustomCursorActive) {
        MouseCursorSystem::first().kill();
    }
    ShowCursor();
}

// raylib's DrawTextureXYZ(RenderTexture.texture) draws upside down.
// This opts for a less confusing approach.
void drawRenderTexture(RenderTexture renderTexture, Color color) {
    const auto tex = renderTexture.texture;
    DrawTextureRec(tex, Rectangle(0, 0, tex.width, -tex.height), Vector2(0, 0), color);
}

Vector2i getMouseWorldPosition() {
    return screenToWorldCoords(System::input.MousePosition);
}

}  // namespace whal
