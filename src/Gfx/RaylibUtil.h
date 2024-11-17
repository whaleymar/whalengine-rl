#pragma once

#include "Util/Vector.h"

typedef struct Color Color;
typedef struct Font Font;
typedef struct RenderTexture RenderTexture;
typedef struct Texture Texture;
typedef struct Rectangle Rectangle;
typedef struct Vector2 Vector2;

namespace whal::gfx {

struct RaylibDrawParams;

void DrawTextBoxed(Font font, const char* text, gfx::RaylibDrawParams params, float fontSize, float spacing, bool wordWrap, bool center, Color tint,
                   float angle, Vector2f pivotOffset);
void DrawTextBoxedSelectable(Font font, const char* text, gfx::RaylibDrawParams params, float fontSize, float spacing, bool wordWrap, bool center,
                             Color tint, int selectStart, int selectLength, Color selectTint, float angle, Vector2f pivotOffset);

RenderTexture LoadRenderTextureDepthTex(int width, int height);
void UnloadRenderTextureDepthTex(RenderTexture target);
void DrawTextureDepth(Texture texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint, float depth);

void DrawRenderTexture(RenderTexture renderTexture, Color color = WHITE);

void DrawPixel(Vector2i screenCoord, Color color);
void DrawEllipse(Vector2f center, Vector2f radii, Color color);
void DrawEllipseFromRect(Rectangle rect, Color color);

// Modified version of DrawTexturePro which doesn't clamp HDR colors
// I can also co-opt the normals RESEARCH
// In the Future Future I should just change the raylib batched vertex buffer to support more custom stuff
void DrawSpriteHDR(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint, float brightness);

// HDR version of DrawRectanglePro
void DrawRectangleHDR(Rectangle rec, Vector2 origin, float rotation, Color color, float brightness);
}  // namespace whal::gfx
