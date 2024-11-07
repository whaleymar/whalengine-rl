#pragma once

#include "Components/Draw.h"
#include "Util/Vector.h"

typedef struct RenderTexture RenderTexture;

// TODO namespace these, I can't remember what they're called
namespace whal {

// Cursor
void setCustomCursor(whal::Sprite drawComponent);
void setDefaultCursor();

// Gfx
void drawRenderTexture(RenderTexture renderTexture, Color color = WHITE);

// Coordinates
Vector2i getMouseWorldPosition();  // TODO put in input system?
}  // namespace whal
