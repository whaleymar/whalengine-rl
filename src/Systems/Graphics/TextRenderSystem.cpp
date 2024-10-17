#include "TextRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"

namespace whal {

static Font DEFAULT_FONT;
static constexpr s32 FONT_SIZE = 40 * VIRTUAL_SCREEN_RATIO / 4.0f;
static void DrawTextBoxed(Font font, const char* text, Rectangle rec, float fontSize, float spacing, bool wordWrap, bool center, Color tint);
static void DrawTextBoxedSelectable(Font font, const char* text, Rectangle rec, float fontSize, float spacing, bool wordWrap, bool center, Color tint,
                                    int selectStart, int selectLength, Color selectTint, Color selectBackTint);

TextRenderSystem::TextRenderSystem() {
    DEFAULT_FONT = LoadFontEx(FONT_PATH, FONT_SIZE, 0, 0);
}

void TextRenderSystem::draw(ecs::Entity entity, const RenderContext ctx) const {
    constexpr s32 spacing = 0;  // PARAM
    const Color tint = WHITE;   // PARAM

    const Transform2D trans = entity.get<Transform2D>();
    const DrawText draw = entity.get<DrawText>();

    Vector2f frameSize = draw.frameSize.as<f32>() * VIRTUAL_SCREEN_RATIO * trans.scale;

    // text is drawn at full resolution
    Vector2f dstPosition = {trans.position.x - ctx.cameraPosition.x, -1 * trans.position.y + ctx.cameraPosition.y};
    dstPosition *= VIRTUAL_SCREEN_RATIO;
    dstPosition += Vector2f(WINDOW_WIDTH_RENDER / 2, WINDOW_HEIGHT_RENDER / 2);

    // drawing one line:
    // Vector2 textDimensions = MeasureTextEx(DEFAULT_FONT, draw.text, FONT_SIZE, spacing);
    // dstPosition -= Vector2f(textDimensions.x / 2, textDimensions.y);
    // DrawTextEx(DEFAULT_FONT, draw.text, Vector2(dstPosition.x, dstPosition.y), FONT_SIZE, spacing, ColorTint(draw.color, tint));

    // TODO rotations
    // drawing wrapped:
    dstPosition -= frameSize * Vector2f(0.5, 1);
    dstPosition +=
        Vector2f(0, FPIXELS_PER_TILE / 2 * VIRTUAL_SCREEN_RATIO);  // needs half tile offset for some reason; might be an issue with map data
    Rectangle dstRect = Rectangle(dstPosition.x, dstPosition.y, frameSize.x, frameSize.y);
    DrawTextBoxed(DEFAULT_FONT, draw.text.c_str(), dstRect, FONT_SIZE, spacing, true, draw.isCentered, ColorTint(draw.color, tint));
}

void TextRenderSystem::addToQueue(std::vector<EntityRenderInfo>& queue) const {
    queue.reserve(getEntitiesMutable().size());  // reserve space in case capacity is too low
    for (auto [entityid, entity] : getEntitiesMutable()) {
        const auto draw = entity.get<DrawText>();

        queue.emplace_back(EntityRenderInfo{
            .boundingBox = AABB(entity.get<Transform2D>(), draw.frameSize / 2),  // TODO this doesn't account for rotations
            .depth = draw.depth,
            .entity = entity,
            .piRender = this,
        });
    }
}

// Draw text using font inside rectangle limits
static void DrawTextBoxed(Font font, const char* text, Rectangle rec, float fontSize, float spacing, bool wordWrap, bool center, Color tint) {
    DrawTextBoxedSelectable(font, text, rec, fontSize, spacing, wordWrap, center, tint, 0, 0, WHITE, WHITE);
}

// Draw text using font inside rectangle limits with support for text selection
static void DrawTextBoxedSelectable(Font font, const char* text, const Rectangle rec, float fontSize, float spacing, bool wordWrap, bool center,
                                    Color tint, int selectStart, int selectLength, Color selectTint, Color selectBackTint) {
    int length = TextLength(text);  // Total length in bytes of the text, scanned by codepoints in loop

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
                    DrawRectangleRec((Rectangle){rec.x + textOffsetX - 1, rec.y + textOffsetY, glyphWidth, (float)font.baseSize * scaleFactor},
                                     selectBackTint);
                    isGlyphSelected = true;
                }

                // Draw current character glyph
                if ((codepoint != ' ') && (codepoint != '\t')) {
                    DrawTextCodepoint(font, codepoint, (Vector2){rec.x + centerOffsetX + textOffsetX, rec.y + textOffsetY}, fontSize,
                                      isGlyphSelected ? selectTint : tint);
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
