#include "RaylibUtil.h"

#include <cstring>
#include <string>

#include "Settings.h"
#include "Sys/System.h"
#include "raylib.h"
#include "rlgl.h"

#include "Systems/Graphics/Common.h"

namespace whal::gfx {

static void DrawTextCodepointPro(rl::Font font, int codepoint, rl::Vector2 position, float fontSize, rl::Vector4 hdrColor, float angle,
                                 rl::Vector2 origin, rl::Vector3 packedCBI, rl::Vector2 scale);

void BeginTextureMode(rl::RenderTexture2D target) {
    rl::BeginTextureMode(target);
    Graphics.globalUniformSetVec2("_Resolution", rl::Vector2(target.texture.width, target.texture.height));
    Graphics.globalUniformSetFloat("_VirtualRatio", static_cast<f32>(target.texture.width) / FWINDOW_WIDTH_GAME);
}

void EndTextureMode() {
    rl::EndTextureMode();
    Graphics.globalUniformSetVec2("_Resolution", rl::Vector2(FWINDOW_WIDTH_STRETCH, FWINDOW_HEIGHT_STRETCH));
    Graphics.globalUniformSetFloat("_VirtualRatio", VIRTUAL_SCREEN_RATIO_STRETCH);
}

// Draw text using font inside rectangle limits
void DrawTextBoxed(rl::Font font, const char* text, RaylibDrawParams params, float fontSize, float spacing, bool wordWrap, bool center, Color tint,
                   float angle, Vector2f pivotOffset, gfx::DrawMetaData cbi, rl::Vector2 scale) {
    DrawTextBoxedSelectable(font, text, params, fontSize, spacing, wordWrap, center, tint, 0, 0, Colors::White, angle, pivotOffset, cbi, scale);
}

// Draw text using font inside rectangle limits with support for text selection
void DrawTextBoxedSelectable(rl::Font font, const char* text, const RaylibDrawParams params, float fontSize, float spacing, bool wordWrap,
                             bool center, Color tint, int selectStart, int selectLength, Color selectTint, float angle, Vector2f pivotOffset,
                             gfx::DrawMetaData cbi, rl::Vector2 scale) {
    int length = rl::TextLength(text);  // Total length in bytes of the text, scanned by codepoints in loop
    const auto rec = params.rect;

    float textOffsetY = 0;     // Offset between lines (on line break '\n')
    float textOffsetX = 0.0f;  // Offset X to next character to draw

    bool isLineMeasureNeeded = true;
    float centerOffsetX = 0.0f;

    // Character rectangle scaling factor
    rl::Vector2 scaleFactor = rl::Vector2{
        fontSize / (float)font.baseSize * scale.x,
        fontSize / (float)font.baseSize * scale.y,

    };

    // Word/character wrapping mechanism variables
    enum { MEASURE_STATE = 0, DRAW_STATE = 1 };
    int state = wordWrap ? MEASURE_STATE : DRAW_STATE;

    int startLine = -1;  // Index where to begin drawing (where a line begins)
    int endLine = -1;    // Index where to stop drawing (where a line ends)
    int lastk = -1;      // Holds last value of the character position

    Vector2f centerpoint = Vector2f(rec.x + rec.width / 2.0f, rec.y - rec.height / 2.0f) + pivotOffset;

    // calc HDR colors and pack the depth info
    rl::Vector4 hdrColor = tint.asRL();
    rl::Vector4 hdrSelectColor = selectTint.asRL();
    auto packedCBI = cbi.asRL();

    for (int i = 0, k = 0; i < length; i++, k++) {
        // Get next codepoint from byte string and glyph index in font
        int codepointByteCount = 0;
        int codepoint = rl::GetCodepoint(&text[i], &codepointByteCount);
        int index = rl::GetGlyphIndex(font, codepoint);

        // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
        // but we need to draw all of the bad bytes using the '?' symbol moving one byte
        if (codepoint == 0x3f)
            codepointByteCount = 1;
        i += (codepointByteCount - 1);

        float glyphWidth = 0;
        if (codepoint != '\n') {
            glyphWidth = (font.glyphs[index].advanceX == 0) ? font.recs[index].width * scaleFactor.x : font.glyphs[index].advanceX * scaleFactor.x;

            if (i + 1 < length)
                glyphWidth = glyphWidth + spacing;
        }

        // NOTE: When wordWrap is ON we first measure how much of the text we can draw before going outside of the rec container
        // We store this info in startLine and endLine, then we change states, draw the text between those two variables
        // and change states again and again recursively until the end of the text (or until we get outside of the container).
        // When wordWrap is OFF we don't need the measure state so we go to the drawing state immediately
        // and begin drawing on the next line before we can get outside the container.
        if (state == MEASURE_STATE) {
            if ((codepoint == ' ') || (codepoint == '\t') || (codepoint == '\n'))
                endLine = i;

            if ((textOffsetX + glyphWidth) > rec.width) {
                endLine = (endLine < 1) ? i : endLine;
                if (i == endLine)
                    endLine -= codepointByteCount;
                if ((startLine + codepointByteCount) == endLine)
                    endLine = (i - codepointByteCount);

                state = !state;
            } else if ((i + 1) == length) {
                endLine = i;
                state = !state;
            } else if (codepoint == '\n')
                state = !state;

            if (state == DRAW_STATE) {
                textOffsetX = 0;
                i = startLine;
                glyphWidth = 0;

                // Save character position when we switch states
                int tmp = lastk;
                lastk = k - 1;
                k = tmp;
            }
        } else {
            if (isLineMeasureNeeded && center) {
                const std::string lineStr = endLine == -1 ? std::string(text).substr(startLine == -1 ? 0 : startLine + 1) :
                                                            std::string(text).substr(startLine == -1 ? 0 : startLine + 1, endLine - startLine);

                rl::Vector2 textDimensions = rl::MeasureTextEx(font, lineStr.c_str(), fontSize, spacing);
                textDimensions.x *= scale.x;
                textDimensions.y *= scale.y;
                centerOffsetX = (rec.width - textDimensions.x) / 2;

                isLineMeasureNeeded = false;
            }
            if (codepoint == '\n') {
                if (!wordWrap) {
                    textOffsetY += (font.baseSize + font.baseSize / 2) * scaleFactor.y;
                    textOffsetX = 0;
                }
            } else {
                if (!wordWrap && ((textOffsetX + glyphWidth) > rec.width)) {
                    textOffsetY += (font.baseSize + font.baseSize / 2) * scaleFactor.y;
                    textOffsetX = 0;
                }

                // When text overflows rectangle height limit, just stop drawing
                if ((textOffsetY + font.baseSize * scaleFactor.y) > rec.height)
                    break;

                // Draw selection background
                bool isGlyphSelected = false;
                if ((selectStart >= 0) && (k >= selectStart) && (k < (selectStart + selectLength))) {
                    isGlyphSelected = true;
                }

                // Draw current character glyph
                if ((codepoint != ' ') && (codepoint != '\t')) {
                    Vector2f pos = Vector2f(rec.x + centerOffsetX + textOffsetX, rec.y + textOffsetY).rotate(-angle, centerpoint);
                    DrawTextCodepointPro(font, codepoint, pos.asRL(), fontSize, isGlyphSelected ? hdrSelectColor : hdrColor, angle, rl::Vector2{0, 0},
                                         packedCBI, scale);
                }
            }

            if (wordWrap && (i == endLine)) {
                // textOffsetY += (font.baseSize + font.baseSize / 2) * scaleFactor;
                textOffsetY += (font.baseSize) * scaleFactor.y;
                textOffsetX = 0;
                startLine = endLine;
                endLine = -1;
                glyphWidth = 0;
                selectStart += lastk - k;
                k = lastk;

                state = !state;
                isLineMeasureNeeded = true;
            }
        }

        if ((textOffsetX != 0) || (codepoint != ' '))
            textOffsetX += glyphWidth;  // avoid leading spaces
    }
}

// added rotation support :)
static void DrawTextCodepointPro(rl::Font font, int codepoint, rl::Vector2 position, float fontSize, rl::Vector4 hdrColor, float angle,
                                 rl::Vector2 origin, rl::Vector3 packedCBI, rl::Vector2 scale) {
    // Character index position in sprite font
    // NOTE: In case a codepoint is not available in the font, index returned points to '?'
    int index = GetGlyphIndex(font, codepoint);
    float scaleFactor = fontSize / font.baseSize;  // Character quad scaling factor
    scale = rl::Vector2{scaleFactor * scale.x, scaleFactor * scale.y};

    // Character destination rectangle on screen
    // NOTE: We consider glyphPadding on drawing
    rl::Rectangle dstRec = {position.x + font.glyphs[index].offsetX * scale.x - (float)font.glyphPadding * scale.x,
                            position.y + font.glyphs[index].offsetY * scale.y - (float)font.glyphPadding * scale.y,
                            (font.recs[index].width + 2.0f * font.glyphPadding) * scale.x,
                            (font.recs[index].height + 2.0f * font.glyphPadding) * scale.y};

    // Character source rectangle from font texture atlas
    // NOTE: We consider chars padding when drawing, it could be required for outline/glow shader effects
    rl::Rectangle srcRec = {font.recs[index].x - (float)font.glyphPadding, font.recs[index].y - (float)font.glyphPadding,
                            font.recs[index].width + 2.0f * font.glyphPadding, font.recs[index].height + 2.0f * font.glyphPadding};

    // Draw the character texture on the screen
    DrawSpriteHDR(font.texture, srcRec, dstRec, origin, angle, hdrColor, packedCBI);
}

// raylib's DrawTextureXYZ(RenderTexture.texture) draws upside down.
// This opts for a less confusing approach.
void DrawRenderTexture(rl::RenderTexture renderTexture, rl::Color color) {
    const auto tex = renderTexture.texture;
    rl::DrawTextureRec(tex, rl::Rectangle(0, 0, tex.width, -tex.height), rl::Vector2(0, 0), color);
}

void DrawRenderTextureCentered(rl::RenderTexture renderTexture, rl::Color color) {
    s32 width = renderTexture.texture.width;
    s32 height = renderTexture.texture.height;
    rl::DrawTexturePro(renderTexture.texture, rl::Rectangle(0, 0, width, -height),
                       rl::Rectangle(WINDOW_POS_OS_X, WINDOW_POS_OS_Y, WINDOW_WIDTH_STRETCH, WINDOW_HEIGHT_STRETCH), rl::Vector2{0, 0}, 0, color);
}

void DrawRenderTextureHDR(rl::RenderTexture renderTexture, Color color) {
    const auto tex = renderTexture.texture;
    DrawSpriteHDR(tex, rl::Rectangle(0, 0, tex.width, -tex.height), rl::Rectangle(0, 0, tex.width, tex.height), rl::Vector2(0, 0), 0.0f, color.asRL(),
                  rl::Vector3(0, 0, 0));
}

void DrawPixel(Vector2f screenCoord, Color color, gfx::DrawMetaData cbi) {
    DrawRectangleHDR(rl::Rectangle{screenCoord.x, screenCoord.y, VIRTUAL_SCREEN_RATIO, VIRTUAL_SCREEN_RATIO}, {0, 0}, 0, color, cbi);
}

// Draws pixelated ellipse even for higher resolution target textures.
void DrawEllipse(Vector2f center, Vector2f radii, Color color, gfx::DrawMetaData cbi) {
    const f32 step = VIRTUAL_SCREEN_RATIO;

    // offset center by subpixel for better distance calculations
    center -= (Vector2f::ONE * VIRTUAL_SCREEN_RATIO / 2.0f);

    radii = radii * VIRTUAL_SCREEN_RATIO;
    Vector2f offset = Vector2f(-1, 0) * VIRTUAL_SCREEN_RATIO;
    const Vector2f lowF = center - radii + offset;
    const Vector2f highF = center + radii;
    const Vector2f denoms = Vector2f(1.0f / (radii.x * radii.x), 1.0f / (radii.y * radii.y));

    Vector2f current = lowF;
    const f32 maxAlpha = color.a;
    for (f32 x = lowF.x; x < highF.x; x += step) {
        for (f32 y = lowF.y; y < highF.y; y += step) {
            f32 xtest = (current.x - center.x);
            xtest = (xtest * xtest) * denoms.x;

            f32 ytest = (current.y - center.y);
            ytest = (ytest * ytest) * denoms.y;

            if ((xtest + ytest) <= 1.0f) {
                f32 distanceFrac = 1.0f - xtest - ytest;
                color.a = (distanceFrac * distanceFrac) * maxAlpha;
                DrawPixel({x, y}, color, cbi);
            }
            current += Vector2f(0, step);
        }
        current.y = lowF.y;
        current += Vector2f(step, 0);
    }
}

void DrawEllipseFromRect(rl::Rectangle rect, Color color, gfx::DrawMetaData cbi) {
    DrawEllipse(Vector2f(rect.x, rect.y), Vector2f(rect.width / 2, rect.height / 2), color, cbi);
}

void DrawSpriteHDR(rl::Texture2D texture, rl::Rectangle source, rl::Rectangle dest, rl::Vector2 origin, float rotation, rl::Vector4 hdrColor,
                   rl::Vector3 packedCBI) {
    // Check if texture is valid
    if (texture.id > 0) {
        float width = (float)texture.width;
        float height = (float)texture.height;

        bool flipX = false;

        if (source.width < 0) {
            flipX = true;
            source.width *= -1;
        }
        if (source.height < 0)
            source.y -= source.height;

        rl::Vector2 topLeft;
        rl::Vector2 topRight;
        rl::Vector2 bottomLeft;
        rl::Vector2 bottomRight;

        // Only calculate rotation if needed
        if (rotation == 0.0f) {
            float x = dest.x - origin.x;
            float y = dest.y - origin.y;
            topLeft = (rl::Vector2){x, y};
            topRight = (rl::Vector2){x + dest.width, y};
            bottomLeft = (rl::Vector2){x, y + dest.height};
            bottomRight = (rl::Vector2){x + dest.width, y + dest.height};
        } else {
            float sinRotation = sinf(rotation * DEG2RAD);
            float cosRotation = cosf(rotation * DEG2RAD);
            float x = dest.x;
            float y = dest.y;
            float dx = -origin.x;
            float dy = -origin.y;

            topLeft.x = x + dx * cosRotation - dy * sinRotation;
            topLeft.y = y + dx * sinRotation + dy * cosRotation;

            topRight.x = x + (dx + dest.width) * cosRotation - dy * sinRotation;
            topRight.y = y + (dx + dest.width) * sinRotation + dy * cosRotation;

            bottomLeft.x = x + dx * cosRotation - (dy + dest.height) * sinRotation;
            bottomLeft.y = y + dx * sinRotation + (dy + dest.height) * cosRotation;

            bottomRight.x = x + (dx + dest.width) * cosRotation - (dy + dest.height) * sinRotation;
            bottomRight.y = y + (dx + dest.width) * sinRotation + (dy + dest.height) * cosRotation;
        }

        rl::rlSetTexture(texture.id);
        rl::rlBegin(RL_QUADS);

        rl::rlColor4f(hdrColor.x, hdrColor.y, hdrColor.z, hdrColor.w);
        rl::rlSetNormals(packedCBI);

        const float texLeft = flipX ? (source.x + source.width) / width : source.x / width;
        const float texRight = flipX ? source.x / width : (source.x + source.width) / width;
        const float texTop = source.y / height;
        const float texBottom = (source.y + source.height) / height;

        // Top-left corner for texture and quad
        rl::rlTexCoord2f(texLeft, texTop);
        rl::rlVertex2f(topLeft.x, topLeft.y);

        // Bottom-left corner for texture and quad
        rl::rlTexCoord2f(texLeft, texBottom);
        rl::rlVertex2f(bottomLeft.x, bottomLeft.y);

        // Bottom-right corner for texture and quad
        rl::rlTexCoord2f(texRight, texBottom);
        rl::rlVertex2f(bottomRight.x, bottomRight.y);

        // Top-right corner for texture and quad
        rl::rlTexCoord2f(texRight, texTop);
        rl::rlVertex2f(topRight.x, topRight.y);

        rl::rlEnd();
        rl::rlSetTexture(0);
    }
}

void DrawRectangleHDR(rl::Rectangle rec, rl::Vector2 origin, float rotation, Color color, gfx::DrawMetaData colorBufInfo) {
    DrawRectangleHDR(rec, origin, rotation, color.asRL(), colorBufInfo.asRL());
}

void DrawRectangleHDR(rl::Rectangle rec, rl::Vector2 origin, float rotation, rl::Vector4 hdrColor, rl::Vector3 packedCBI) {
    rl::Vector2 topLeft = {};
    rl::Vector2 topRight = {};
    rl::Vector2 bottomLeft = {};
    rl::Vector2 bottomRight = {};

    // Only calculate rotation if needed
    if (rotation == 0.0f) {
        float x = rec.x - origin.x;
        float y = rec.y - origin.y;
        topLeft = (rl::Vector2){x, y};
        topRight = (rl::Vector2){x + rec.width, y};
        bottomLeft = (rl::Vector2){x, y + rec.height};
        bottomRight = (rl::Vector2){x + rec.width, y + rec.height};
    } else {
        float sinRotation = sinf(rotation * DEG2RAD);
        float cosRotation = cosf(rotation * DEG2RAD);
        float x = rec.x;
        float y = rec.y;
        float dx = -origin.x;
        float dy = -origin.y;

        topLeft.x = x + dx * cosRotation - dy * sinRotation;
        topLeft.y = y + dx * sinRotation + dy * cosRotation;

        topRight.x = x + (dx + rec.width) * cosRotation - dy * sinRotation;
        topRight.y = y + (dx + rec.width) * sinRotation + dy * cosRotation;

        bottomLeft.x = x + dx * cosRotation - (dy + rec.height) * sinRotation;
        bottomLeft.y = y + dx * sinRotation + (dy + rec.height) * cosRotation;

        bottomRight.x = x + (dx + rec.width) * cosRotation - (dy + rec.height) * sinRotation;
        bottomRight.y = y + (dx + rec.width) * sinRotation + (dy + rec.height) * cosRotation;
    }

#if defined(SUPPORT_QUADS_DRAW_MODE)
    rlSetTexture(GetShapesTexture().id);
    Rectangle shapeRect = GetShapesTextureRectangle();

    rlBegin(RL_QUADS);

    rlSetNormals(packedCBI);
    rlColor4f(hdrColor.x, hdrColor.y, hdrColor.z, hdrColor.w);

    rlTexCoord2f(shapeRect.x / texShapes.width, shapeRect.y / texShapes.height);
    rlVertex2f(topLeft.x, topLeft.y);

    rlTexCoord2f(shapeRect.x / texShapes.width, (shapeRect.y + shapeRect.height) / texShapes.height);
    rlVertex2f(bottomLeft.x, bottomLeft.y);

    rlTexCoord2f((shapeRect.x + shapeRect.width) / texShapes.width, (shapeRect.y + shapeRect.height) / texShapes.height);
    rlVertex2f(bottomRight.x, bottomRight.y);

    rlTexCoord2f((shapeRect.x + shapeRect.width) / texShapes.width, shapeRect.y / texShapes.height);
    rlVertex2f(topRight.x, topRight.y);

    rlEnd();

    rlSetTexture(0);
#else
    rl::rlBegin(RL_TRIANGLES);

    rl::rlSetNormals(packedCBI);
    rl::rlColor4f(hdrColor.x, hdrColor.y, hdrColor.z, hdrColor.w);

    rl::rlVertex2f(topLeft.x, topLeft.y);
    rl::rlVertex2f(bottomLeft.x, bottomLeft.y);
    rl::rlVertex2f(topRight.x, topRight.y);

    rl::rlVertex2f(topRight.x, topRight.y);
    rl::rlVertex2f(bottomLeft.x, bottomLeft.y);
    rl::rlVertex2f(bottomRight.x, bottomRight.y);

    rl::rlEnd();
#endif
}

// Draw a triangle strip defined by points
// NOTE: Every new vertex connects with previous two
static void DrawTriangleStripHDR(const rl::Vector2* points, int pointCount, rl::Vector4 hdrColor, rl::Vector3 packedCBI) {
    if (pointCount >= 3) {
        rl::rlBegin(RL_TRIANGLES);
        rl::rlColor4f(hdrColor.x, hdrColor.y, hdrColor.z, hdrColor.w);
        rl::rlSetNormals(packedCBI);

        for (int i = 2; i < pointCount; i++) {
            if ((i % 2) == 0) {
                rl::rlVertex2f(points[i].x, points[i].y);
                rl::rlVertex2f(points[i - 2].x, points[i - 2].y);
                rl::rlVertex2f(points[i - 1].x, points[i - 1].y);
            } else {
                rl::rlVertex2f(points[i].x, points[i].y);
                rl::rlVertex2f(points[i - 1].x, points[i - 1].y);
                rl::rlVertex2f(points[i - 2].x, points[i - 2].y);
            }
        }
        rl::rlEnd();
    }
}

void DrawLineHDR(rl::Vector2 startPos, rl::Vector2 endPos, float thick, Color color, gfx::DrawMetaData cbi) {
    rl::Vector2 delta = {endPos.x - startPos.x, endPos.y - startPos.y};
    float length = sqrtf(delta.x * delta.x + delta.y * delta.y);

    if ((length > 0) && (thick > 0)) {
        float scale = thick / (2 * length);

        rl::Vector2 radius = {-scale * delta.y, scale * delta.x};
        rl::Vector2 strip[4] = {{startPos.x - radius.x, startPos.y - radius.y},
                                {startPos.x + radius.x, startPos.y + radius.y},
                                {endPos.x - radius.x, endPos.y - radius.y},
                                {endPos.x + radius.x, endPos.y + radius.y}};

        DrawTriangleStripHDR(strip, 4, color.asRL(), cbi.asRL());
    }
}

// Draw spline segment: Quadratic Bezier, 2 points, 1 control point
void DrawSplineSegmentBezierQuadraticHDR(rl::Vector2 p1, rl::Vector2 c2, rl::Vector2 p3, float thick, Color color, gfx::DrawMetaData cbi) {
    constexpr s32 SPLINE_SEGMENT_DIVISIONS = 24;
    const float step = 1.0f / SPLINE_SEGMENT_DIVISIONS;

    rl::Vector2 previous = p1;
    rl::Vector2 current = {0, 0};
    float t = 0.0f;

    rl::Vector2 points[2 * SPLINE_SEGMENT_DIVISIONS + 2];
    std::memset(points, 0, sizeof(points));

    for (int i = 1; i <= SPLINE_SEGMENT_DIVISIONS; i++) {
        t = step * (float)i;

        float a = powf(1.0f - t, 2);
        float b = 2.0f * (1.0f - t) * t;
        float c = powf(t, 2);

        // NOTE: The easing functions aren't suitable here because they don't take a control point
        current.y = a * p1.y + b * c2.y + c * p3.y;
        current.x = a * p1.x + b * c2.x + c * p3.x;

        float dy = current.y - previous.y;
        float dx = current.x - previous.x;
        float size = 0.5f * thick / sqrtf(dx * dx + dy * dy);

        if (i == 1) {
            points[0].x = previous.x + dy * size;
            points[0].y = previous.y - dx * size;
            points[1].x = previous.x - dy * size;
            points[1].y = previous.y + dx * size;
        }

        points[2 * i + 1].x = current.x - dy * size;
        points[2 * i + 1].y = current.y + dx * size;
        points[2 * i].x = current.x + dy * size;
        points[2 * i].y = current.y - dx * size;

        previous = current;
    }

    DrawTriangleStripHDR(points, 2 * SPLINE_SEGMENT_DIVISIONS + 2, color.asRL(), cbi.asRL());
}

}  // namespace whal::gfx
