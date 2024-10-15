#pragma once

#include "Components/Draw.h"
#include "Util/Vector.h"

typedef struct RenderTexture RenderTexture;

// Cursor
void setCustomCursor(whal::Sprite drawComponent);
void setDefaultCursor();

// Gfx
void drawRenderTexture(RenderTexture renderTexture, Color color = WHITE);

// Coordinates
Vector2i getMouseWorldPosition();
