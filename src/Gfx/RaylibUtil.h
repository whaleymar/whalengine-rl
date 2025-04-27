#pragma once

#include "Gfx/Color.h"
#include "Systems/Graphics/Common.h"
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
struct DrawMetaData;

// Thin wrappers over raylib's Begin/EndTextureMode which set a global uniform for the viewport size.
void BeginTextureMode(rl::RenderTexture2D target);
void EndTextureMode();

void DrawTextBoxed(rl::Font font, const char* text, gfx::RaylibDrawParams params, float fontSize, float spacing, bool wordWrap, bool center,
                   Color tint, float angle, Vector2f pivotOffset, gfx::DrawMetaData cbi, rl::Vector2 scale);
void DrawTextBoxedSelectable(rl::Font font, const char* text, gfx::RaylibDrawParams params, float fontSize, float spacing, bool wordWrap, bool center,
                             Color tint, int selectStart, int selectLength, Color selectTint, float angle, Vector2f pivotOffset,
                             gfx::DrawMetaData cbi, rl::Vector2 scale);

void DrawRenderTexture(rl::RenderTexture renderTexture, rl::Color color = rl::WHITE);
void DrawRenderTextureHDR(rl::RenderTexture renderTexture, Color color = Colors::White);
void DrawRenderTextureCentered(rl::RenderTexture renderTexture, rl::Color color = rl::WHITE);

////////////////////////////
// CUSTOM SHAPE FUNCTIONS //
////////////////////////////
void DrawPixel(Vector2f screenCoord, Color color, gfx::DrawMetaData cbi = gfx::DrawMetaData::NONE);
void DrawEllipse(Vector2f center, Vector2f radii, Color color, gfx::DrawMetaData cbi);
void DrawEllipseFromRect(rl::Rectangle rect, Color color, gfx::DrawMetaData cbi);

// Modified version of DrawTexturePro which doesn't clamp HDR colors
// I can also co-opt the normals RESEARCH
// In the Future Future I should just change the raylib batched vertex buffer to support more custom stuff
void DrawSpriteHDR(rl::Texture2D texture, rl::Rectangle source, rl::Rectangle dest, rl::Vector2 origin, float rotation, rl::Vector4 hdrColor,
                   rl::Vector3 packedCBI = rl::Vector3{0, 0, 0}, f32 custom0b = 0.0f, f32 custom0a = 0.0f, bool passSpriteCenter = false);

// HDR version of DrawRectanglePro
void DrawRectangleHDR(rl::Rectangle rec, rl::Vector2 origin, float rotation, Color color, gfx::DrawMetaData colorBufInfo);
void DrawRectangleHDR(rl::Rectangle rec, rl::Vector2 origin, float rotation, rl::Vector4 hdrColor, rl::Vector3 packedCBI);

// HDR version of DrawLineEx
void DrawLineHDR(rl::Vector2 startPos, rl::Vector2 endPos, float thick, Color color, gfx::DrawMetaData cbi);
void DrawSplineSegmentBezierQuadraticHDR(rl::Vector2 p1, rl::Vector2 c2, rl::Vector2 p3, float thick, Color color, gfx::DrawMetaData cbi);

}  // namespace gfx
}  // namespace whal
