#include "EngineUtil.h"

#include <raylib.h>
#include "Components/Callback.h"
#include "Gfx/Coordinates.h"
#include "Sys/System.h"
#include "whalECS/src/ECS.h"

#include "Components/Name.h"

using namespace whal;

static ecs::Entity sCursorEntity;
static bool isCursorAlive = false;

void setCustomCursor(Draw drawComponent) {
    if (!isCursorAlive) {
        sCursorEntity = System::world.entity().value();
        isCursorAlive = true;
        sCursorEntity.add(Name("Cursor"));
        sCursorEntity.add<Transform2D>();
        sCursorEntity.add<CustomUpdate>({[](ecs::Entity self) {
            Vector2i position = getMouseWorldPosition();

            // correct for cursor height
            const auto height = self.get<Draw>().getFrameSize().y;
            position -= Vector2i(0, height / 2);

            // update transform
            self.get<Transform2D>().position = position;
        }});
    } else if (sCursorEntity.has<Draw>()) {
        sCursorEntity.remove<Draw>();
    }

    sCursorEntity.add(drawComponent);
    HideCursor();
}

void setDefaultCursor() {
    if (isCursorAlive) {
        sCursorEntity.kill();
        isCursorAlive = false;
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
