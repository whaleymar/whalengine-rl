#pragma once

#include "Components/Draw.h"
#include "Util/Vector.h"

typedef struct RenderTexture RenderTexture;

// Cursor
void setCustomCursor(whal::Draw drawComponent);
void setDefaultCursor();

// Gfx
void drawRenderTexture(RenderTexture renderTexture);

// Coordinates
Vector2i getMouseWorldPosition();
