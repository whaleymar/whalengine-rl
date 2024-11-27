#pragma once

#include "Util/Vector.h"

namespace rl {
typedef struct Color Color;
typedef struct Font Font;
typedef struct RenderTexture RenderTexture;
typedef struct Texture Texture;
typedef struct Rectangle Rectangle;
typedef struct Vector2 Vector2;
}  // namespace rl

namespace whal {

struct MultiTexture;
enum class TextureID;

namespace gfx {

struct RaylibDrawParams;
struct ColorBufInfo;

void DrawTextBoxed(rl::Font font, const char* text, gfx::RaylibDrawParams params, float fontSize, float spacing, bool wordWrap, bool center,
                   rl::Color tint, float angle, Vector2f pivotOffset, float brightness, gfx::ColorBufInfo cbi);
void DrawTextBoxedSelectable(rl::Font font, const char* text, gfx::RaylibDrawParams params, float fontSize, float spacing, bool wordWrap, bool center,
                             rl::Color tint, int selectStart, int selectLength, rl::Color selectTint, float angle, Vector2f pivotOffset,
                             float brightness, gfx::ColorBufInfo cbi);

void DrawRenderTexture(rl::RenderTexture renderTexture, rl::Color color = rl::WHITE);
void DrawRenderTextureHDR(rl::RenderTexture renderTexture, rl::Vector4 color);

////////////////////////////
// CUSTOM SHAPE FUNCTIONS //
////////////////////////////
void DrawPixel(Vector2i screenCoord, rl::Color color);  // draws w/ LDR color and no depth information
void DrawPixel(Vector2i screenCoord, rl::Color color, float brightness, gfx::ColorBufInfo cbi);
void DrawPixel(Vector2i screenCoord, rl::Vector4 hdrColor, rl::Vector3 packedCBI);
void DrawEllipse(Vector2f center, Vector2f radii, rl::Color color, float brightness, gfx::ColorBufInfo cbi);
void DrawEllipseFromRect(rl::Rectangle rect, rl::Color color, float brightness, gfx::ColorBufInfo cbi);

// Modified version of DrawTexturePro which doesn't clamp HDR colors
// I can also co-opt the normals RESEARCH
// In the Future Future I should just change the raylib batched vertex buffer to support more custom stuff
void DrawSpriteHDR(rl::Texture2D texture, rl::Rectangle source, rl::Rectangle dest, rl::Vector2 origin, float rotation, rl::Color tint,
                   float brightness, gfx::ColorBufInfo colorBufInfo);
void DrawSpriteHDR(rl::Texture2D texture, rl::Rectangle source, rl::Rectangle dest, rl::Vector2 origin, float rotation, rl::Vector4 hdrColor,
                   rl::Vector3 packedCBI);

// HDR version of DrawRectanglePro
void DrawRectangleHDR(rl::Rectangle rec, rl::Vector2 origin, float rotation, rl::Color color, float brightness, gfx::ColorBufInfo colorBufInfo);
void DrawRectangleHDR(rl::Rectangle rec, rl::Vector2 origin, float rotation, rl::Vector4 hdrColor, rl::Vector3 packedCBI);

// HDR version of DrawLineEx
void DrawLineHDR(rl::Vector2 startPos, rl::Vector2 endPos, float thick, rl::Color color, float brightness, gfx::ColorBufInfo cbi);
void DrawSplineSegmentBezierQuadraticHDR(rl::Vector2 p1, rl::Vector2 c2, rl::Vector2 p3, float thick, rl::Color color, float brightness,
                                         gfx::ColorBufInfo cbi);

// Adds extra texture targets for depth and occlusion buffers
MultiTexture CreateMultiTexture();
}  // namespace gfx
}  // namespace whal
