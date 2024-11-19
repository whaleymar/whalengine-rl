#pragma once

#include "Util/Vector.h"

typedef struct Color Color;
typedef struct Font Font;
typedef struct RenderTexture RenderTexture;
typedef struct Texture Texture;
typedef struct Rectangle Rectangle;
typedef struct Vector2 Vector2;

namespace whal {

struct MultiTexture;

namespace gfx {

struct RaylibDrawParams;
struct ColorBufInfo;

void DrawTextBoxed(Font font, const char* text, gfx::RaylibDrawParams params, float fontSize, float spacing, bool wordWrap, bool center, Color tint,
                   float angle, Vector2f pivotOffset, float brightness, gfx::ColorBufInfo cbi);
void DrawTextBoxedSelectable(Font font, const char* text, gfx::RaylibDrawParams params, float fontSize, float spacing, bool wordWrap, bool center,
                             Color tint, int selectStart, int selectLength, Color selectTint, float angle, Vector2f pivotOffset, float brightness,
                             gfx::ColorBufInfo cbi);

RenderTexture LoadRenderTextureDepthTex(int width, int height);
void UnloadRenderTextureDepthTex(RenderTexture target);
void DrawTextureDepth(Texture texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint, float depth);

void DrawRenderTexture(RenderTexture renderTexture, Color color = WHITE);

////////////////////////////
// CUSTOM SHAPE FUNCTIONS //
////////////////////////////
void DrawPixel(Vector2i screenCoord, Color color);  // draws w/ LDR color and no depth information
void DrawPixel(Vector2i screenCoord, Color color, float brightness, gfx::ColorBufInfo cbi);
void DrawPixel(Vector2i screenCoord, Vector4 hdrColor, Vector3 packedCBI);
void DrawEllipse(Vector2f center, Vector2f radii, Color color, float brightness, gfx::ColorBufInfo cbi);
void DrawEllipseFromRect(Rectangle rect, Color color, float brightness, gfx::ColorBufInfo cbi);

// Modified version of DrawTexturePro which doesn't clamp HDR colors
// I can also co-opt the normals RESEARCH
// In the Future Future I should just change the raylib batched vertex buffer to support more custom stuff
void DrawSpriteHDR(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Color tint, float brightness,
                   gfx::ColorBufInfo colorBufInfo);
void DrawSpriteHDR(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, Vector4 hdrColor, Vector3 packedCBI);

// HDR version of DrawRectanglePro
void DrawRectangleHDR(Rectangle rec, Vector2 origin, float rotation, Color color, float brightness, gfx::ColorBufInfo colorBufInfo);
void DrawRectangleHDR(Rectangle rec, Vector2 origin, float rotation, Vector4 hdrColor, Vector3 packedCBI);

// HDR version of DrawLineEx
void DrawLineHDR(Vector2 startPos, Vector2 endPos, float thick, Color color, float brightness, gfx::ColorBufInfo cbi);
void DrawSplineSegmentBezierQuadraticHDR(Vector2 p1, Vector2 c2, Vector2 p3, float thick, Color color, float brightness, gfx::ColorBufInfo cbi);

// Adds extra texture targets for depth and occlusion buffers
MultiTexture CreateMultiTexture();
}  // namespace gfx
}  // namespace whal
