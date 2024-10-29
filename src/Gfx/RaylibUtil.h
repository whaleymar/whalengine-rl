#pragma once

#include "Util/Vector.h"

typedef struct Color Color;
typedef struct Font Font;
typedef struct RenderTexture RenderTexture;
typedef struct Texture Texture;
typedef struct Rectangle Rectangle;
typedef struct Vector2 Vector2;

namespace whal {

struct RaylibDrawParams;

void DrawTextBoxed(Font font, const char* text, RaylibDrawParams params, float fontSize, float spacing, bool wordWrap, bool center, Color tint,
                   float angle, Vector2f pivotOffset);
void DrawTextBoxedSelectable(Font font, const char* text, RaylibDrawParams params, float fontSize, float spacing, bool wordWrap, bool center,
                             Color tint, int selectStart, int selectLength, Color selectTint, float angle, Vector2f pivotOffset);

RenderTexture LoadRenderTextureDepthTex(int width, int height);
void UnloadRenderTextureDepthTex(RenderTexture target);
void DrawTextureDepth(Texture texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint, float depth);

}  // namespace whal
