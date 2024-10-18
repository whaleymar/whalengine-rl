#include "TextRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Physics/Box.h"
#include "Settings.h"
#include "Util/Vector.h"
#include "raylib/src/raylib.h"

namespace whal {

static Font DEFAULT_FONT;
static constexpr s32 FONT_SIZE = 40 * VIRTUAL_SCREEN_RATIO / 4.0f;
static void DrawTextBoxed(Font font, const char* text, RaylibDrawParams params, float fontSize, float spacing, bool wordWrap, bool center, Color tint,
                          float angle, Vector2f pivotOffset);
static void DrawTextBoxedSelectable(Font font, const char* text, RaylibDrawParams params, float fontSize, float spacing, bool wordWrap, bool center,
                                    Color tint, int selectStart, int selectLength, Color selectTint, float angle, Vector2f pivotOffset);

TextRenderSystem::TextRenderSystem() {
    DEFAULT_FONT = LoadFontEx(FONT_PATH, FONT_SIZE, 0, 0);
}

void TextRenderSystem::draw(ecs::Entity entity, const RenderContext ctx) const {
    constexpr s32 spacing = 0;  // PARAM
    const Color tint = WHITE;   // PARAM

    const Transform2D trans = entity.get<Transform2D>();
    const DrawText draw = entity.get<DrawText>();

    PreciseTransform2D pTrans = PreciseTransform2D::fromTrans(trans);
    if (entity.has<PreciseTransform2D>()) {
        pTrans.position = entity.get<PrecisePosition>().position;
    }

    // rotation pivot correction
    Vector2f pivotOffsetScreen = pTrans.pivotOffset.as<f32>() * Vector2f(1, -1) * VIRTUAL_SCREEN_RATIO;
    Vector2f frameSize = draw.frameSize.as<f32>();

    // scale to full resolution
    frameSize = frameSize * VIRTUAL_SCREEN_RATIO * pTrans.scale;
    Vector2f dstPosition = {pTrans.position.x - ctx.cameraPosition.x, -1 * pTrans.position.y + ctx.cameraPosition.y};
    dstPosition *= VIRTUAL_SCREEN_RATIO;
    dstPosition += Vector2f(WINDOW_WIDTH_RENDER / 2, WINDOW_HEIGHT_RENDER / 2);

    // UNUSED
    // {
    //  drawing one line:
    //  Vector2 textDimensions = MeasureTextEx(DEFAULT_FONT, draw.text, FONT_SIZE, spacing);
    //  dstPosition -= Vector2f(textDimensions.x / 2, textDimensions.y);
    //  DrawTextEx(DEFAULT_FONT, draw.text, Vector2(dstPosition.x, dstPosition.y), FONT_SIZE, spacing, ColorTint(draw.color, tint));
    // }

    // TODO needs adjustment based on rotation? Currently not working for multi-line text
    // drawing wrapped:
    dstPosition -= frameSize * Vector2f(0.5, 1);  // original
    // needs half tile offset for some reason; might be an issue with map data:
    dstPosition += Vector2f(0, FPIXELS_PER_TILE / 2 * VIRTUAL_SCREEN_RATIO);

    Rectangle dstRect = Rectangle(dstPosition.x, dstPosition.y, frameSize.x, frameSize.y);

    RaylibDrawParams params = RaylibDrawParams{
        .rect = dstRect,
        .origin = Vector2{0, 0},
        .position = toRaylib(dstPosition),
    };
    DrawTextBoxed(DEFAULT_FONT, draw.text.c_str(), params, FONT_SIZE, spacing, true, draw.isCentered, ColorTint(draw.color, tint),
                  pTrans.rotationDegrees, pivotOffsetScreen);
}

void TextRenderSystem::addToQueue(std::vector<EntityRenderInfo>& queue) const {
    queue.reserve(getEntitiesMutable().size());  // reserve space in case capacity is too low
    for (auto [entityid, entity] : getEntitiesMutable()) {
        const auto draw = entity.get<DrawText>();
        const auto trans = entity.get<Transform2D>();

        const auto bb = trans.rotationDegrees == 0.0f ? AABB(trans, draw.frameSize / 2) :
                                                        Box(trans.getRotatedPosition(), draw.frameSize / 2, trans.rotationDegrees).getBoundingAABB();

        queue.emplace_back(EntityRenderInfo{
            .boundingBox = bb,
            .depth = draw.depth,
            .entity = entity,
            .piRender = this,
        });
    }
}

// Draw text using font inside rectangle limits
static void DrawTextBoxed(Font font, const char* text, RaylibDrawParams params, float fontSize, float spacing, bool wordWrap, bool center, Color tint,
                          float angle, Vector2f pivotOffset) {
    DrawTextBoxedSelectable(font, text, params, fontSize, spacing, wordWrap, center, tint, 0, 0, WHITE, angle, pivotOffset);
}

// added rotation support :)
static void DrawTextCodepointPro(Font font, int codepoint, Vector2 position, float fontSize, Color tint, float angle, Vector2 origin) {
    // Character index position in sprite font
    // NOTE: In case a codepoint is not available in the font, index returned points to '?'
    int index = GetGlyphIndex(font, codepoint);
    float scaleFactor = fontSize / font.baseSize;  // Character quad scaling factor

    // Character destination rectangle on screen
    // NOTE: We consider glyphPadding on drawing
    Rectangle dstRec = {position.x + font.glyphs[index].offsetX * scaleFactor - (float)font.glyphPadding * scaleFactor,
                        position.y + font.glyphs[index].offsetY * scaleFactor - (float)font.glyphPadding * scaleFactor,
                        (font.recs[index].width + 2.0f * font.glyphPadding) * scaleFactor,
                        (font.recs[index].height + 2.0f * font.glyphPadding) * scaleFactor};

    // Character source rectangle from font texture atlas
    // NOTE: We consider chars padding when drawing, it could be required for outline/glow shader effects
    Rectangle srcRec = {font.recs[index].x - (float)font.glyphPadding, font.recs[index].y - (float)font.glyphPadding,
                        font.recs[index].width + 2.0f * font.glyphPadding, font.recs[index].height + 2.0f * font.glyphPadding};

    // Draw the character texture on the screen
    DrawTexturePro(font.texture, srcRec, dstRec, origin, angle, tint);
}

// Draw text using font inside rectangle limits with support for text selection
static void DrawTextBoxedSelectable(Font font, const char* text, const RaylibDrawParams params, float fontSize, float spacing, bool wordWrap,
                                    bool center, Color tint, int selectStart, int selectLength, Color selectTint, float angle, Vector2f pivotOffset) {
    int length = TextLength(text);  // Total length in bytes of the text, scanned by codepoints in loop
    const auto rec = params.rect;

    float textOffsetY = 0;     // Offset between lines (on line break '\n')
    float textOffsetX = 0.0f;  // Offset X to next character to draw

    bool isLineMeasureNeeded = true;
    float centerOffsetX = 0.0f;

    float scaleFactor = fontSize / (float)font.baseSize;  // Character rectangle scaling factor

    // Word/character wrapping mechanism variables
    enum { MEASURE_STATE = 0, DRAW_STATE = 1 };
    int state = wordWrap ? MEASURE_STATE : DRAW_STATE;

    int startLine = -1;  // Index where to begin drawing (where a line begins)
    int endLine = -1;    // Index where to stop drawing (where a line ends)
    int lastk = -1;      // Holds last value of the character position

    Vector2f centerpoint = Vector2f(rec.x + rec.width / 2.0f, rec.y - rec.height / 2.0f) + pivotOffset;

    for (int i = 0, k = 0; i < length; i++, k++) {
        // Get next codepoint from byte string and glyph index in font
        int codepointByteCount = 0;
        int codepoint = GetCodepoint(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
        // but we need to draw all of the bad bytes using the '?' symbol moving one byte
        if (codepoint == 0x3f)
            codepointByteCount = 1;
        i += (codepointByteCount - 1);

        float glyphWidth = 0;
        if (codepoint != '\n') {
            glyphWidth = (font.glyphs[index].advanceX == 0) ? font.recs[index].width * scaleFactor : font.glyphs[index].advanceX * scaleFactor;

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

                Vector2 textDimensions = MeasureTextEx(DEFAULT_FONT, lineStr.c_str(), FONT_SIZE, spacing);
                centerOffsetX = (rec.width - textDimensions.x) / 2;

                isLineMeasureNeeded = false;
            }
            if (codepoint == '\n') {
                if (!wordWrap) {
                    textOffsetY += (font.baseSize + font.baseSize / 2) * scaleFactor;
                    textOffsetX = 0;
                }
            } else {
                if (!wordWrap && ((textOffsetX + glyphWidth) > rec.width)) {
                    textOffsetY += (font.baseSize + font.baseSize / 2) * scaleFactor;
                    textOffsetX = 0;
                }

                // When text overflows rectangle height limit, just stop drawing
                if ((textOffsetY + font.baseSize * scaleFactor) > rec.height)
                    break;

                // Draw selection background
                bool isGlyphSelected = false;
                if ((selectStart >= 0) && (k >= selectStart) && (k < (selectStart + selectLength))) {
                    isGlyphSelected = true;
                }

                // Draw current character glyph
                if ((codepoint != ' ') && (codepoint != '\t')) {
                    Vector2f pos = Vector2f(rec.x + centerOffsetX + textOffsetX, rec.y + textOffsetY).rotate(-angle, centerpoint);
                    DrawTextCodepointPro(font, codepoint, toRaylib(pos), fontSize, isGlyphSelected ? selectTint : tint, angle, Vector2{0, 0});
                }
            }

            if (wordWrap && (i == endLine)) {
                // textOffsetY += (font.baseSize + font.baseSize / 2) * scaleFactor;
                textOffsetY += (font.baseSize) * scaleFactor;
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

}  // namespace whal
