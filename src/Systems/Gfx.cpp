#include "Gfx.h"

#include <algorithm>
#include <raylib.h>

#include "Components/Tags.h"
#include "Game.h"
#include "Gfx/Depth.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Settings.h"

#include "Systems/TagTrackers.h"
#include "Util/Vector.h"

#include "Components/Draw.h"
#include "Components/Transform.h"

namespace whal {

static void DrawTextBoxed(Font font, const char* text, Rectangle rec, float fontSize, float spacing, bool wordWrap, bool center, Color tint);
static void DrawTextBoxedSelectable(Font font, const char* text, Rectangle rec, float fontSize, float spacing, bool wordWrap, bool center, Color tint,
                                    int selectStart, int selectLength, Color selectTint, Color selectBackTint);

static Font DEFAULT_FONT;
static const s32 FONT_SIZE = 40 * VIRTUAL_SCREEN_RATIO / 4.0f;

void GfxSystem::onAdd(const ecs::Entity entity) {
    const auto draw = entity.get<Draw>();
    const DrawInfo drawInfo(entity, depthToFloat(draw.getDepth()), static_cast<s16>(draw.getShader()));
    mAddedEntities.push_back(drawInfo);
}

void GfxSystem::onRemove(const ecs::Entity entity) {
    auto pred = [entity](const DrawInfo& drawInfo) { return drawInfo.entity.id() == entity.id(); };
    auto numRemoved = mSorted.remove_if(pred);
    if (numRemoved == 0) {
        auto it = std::find_if(mAddedEntities.begin(), mAddedEntities.end(), pred);
        assert(it != mAddedEntities.end() && "tried to remove entity from GfxSystem, but couldn't find it in mSorted or mAddedEntities collections");
        mAddedEntities.erase(it);
    }
}

// returns true if `first` should be drawn before `seccond`
// based on depth, then shader
bool GfxSystem::isBelow(const DrawInfo& first, const DrawInfo& second) {
    return !(first.depth > second.depth || (first.depth == second.depth && first.shaderIx >= second.shaderIx));
}

void GfxSystem::drawEntities() {
    auto cameraPosF = getCameraPositionPrecise();
    // auto cameraPosF = toFloatVec(getCameraPosition());

    const Texture2D& spriteTexture = TextureManager::instance().getTextureAtlas(TEXNAME_SPRITE).getTexture();

    std::sort(mAddedEntities.begin(), mAddedEntities.end(), &isBelow);

    auto it = mSorted.before_begin();
    auto current = mSorted.begin();
    auto insertIt = mAddedEntities.begin();

    // insert new elements in sorted order:
    while (current != mSorted.end()) {
        while (insertIt != mAddedEntities.end() && isBelow(*insertIt, *current)) {
            it = mSorted.insert_after(it, *insertIt);
            ++insertIt;
        }
        ++it;
        ++current;
    }
    // insert remaining elements in vec:
    while (insertIt != mAddedEntities.end()) {
        it = mSorted.insert_after(it, *insertIt);
        ++insertIt;
    }

    mAddedEntities.clear();

    Shaders prevShader = static_cast<Shaders>(mSorted.begin()->shaderIx);
    BeginShaderMode(ShaderManager::get(prevShader));
    for (auto const drawInfo : mSorted) {
        if (drawInfo.entity.has<Invisible>()) {
            continue;
        }
        Shaders newShader = static_cast<Shaders>(drawInfo.shaderIx);
        if (newShader != prevShader) {
            prevShader = newShader;
            EndShaderMode();
            BeginShaderMode(ShaderManager::get(newShader));
        }
        drawEntity(drawInfo.entity, spriteTexture, cameraPosF);
    }
    EndShaderMode();
}

void GfxSystem::drawEntity(ecs::Entity entity, const Texture2D& spriteTexture, const Vector2f cameraPosF) {
    const Transform2D trans = entity.get<Transform2D>();
    Vector2f posF = trans.position.as<f32>();
    Draw draw = entity.get<Draw>();

    if (draw.getTag() == Draw::DrawTag::Rect) {
        const DrawRect rect = draw.getRect();
        auto frameSize = rect.getFrameSizeTexels().as<f32>();
        Vector2f dstSize = {frameSize.x * rect.scale.x * FPIXELS_PER_TEXEL, frameSize.y * rect.scale.y * FPIXELS_PER_TEXEL};

        // subtract size.y so we draw from bottom left instead of top left
        Vector2f dstPosition = {trans.position.x - cameraPosF.x, -1 * trans.position.y + cameraPosF.y - dstSize.y};
        // add halfX to pos to match the origin thingy done w/ sprites
        dstPosition -= {dstSize.x * 0.5f, 0};
        Rectangle dstRect = Rectangle(dstPosition.x, dstPosition.y, dstSize.x, dstSize.y);
        DrawRectangleRec(dstRect, rect.color);

    } else {
        const Sprite sprite = draw.getSprite();
        const Vector2i frameSize = sprite.getFrameSizeTexels();
        const s32 flipModifier = trans.facing == Facing::Left ? -1 : 1;
        const Rectangle srcRect = Rectangle(sprite.atlasPositionTexels.x, sprite.atlasPositionTexels.y, flipModifier * frameSize.x, frameSize.y);

        Vector2f dstSize = {frameSize.x * sprite.scale.x * FPIXELS_PER_TEXEL, frameSize.y * sprite.scale.y * FPIXELS_PER_TEXEL};
        Vector2f dstPosition = {posF.x - cameraPosF.x, -1.0f * posF.y + cameraPosF.y};

        // rotate about center or transform
        Vector2f origin = sprite.isRotateAboutCenter ? dstSize * Vector2f(0.5, 0.5) : Vector2f(dstSize.x * 0.5, dstSize.y);
        if (sprite.isRotateAboutCenter) {
            dstPosition -= Vector2f(0, dstSize.y / 2.0f);
        }

        Rectangle dstRect = Rectangle(dstPosition.x, dstPosition.y, dstSize.x, dstSize.y);
        DrawTexturePro(spriteTexture, srcRect, dstRect, {origin.x, origin.y}, trans.rotationDegrees, sprite.color);
    }
}

DrawTextSystem::DrawTextSystem() {
    DEFAULT_FONT = LoadFontEx(FONT_PATH, FONT_SIZE, 0, 0);
}

void DrawTextSystem::drawEntities(Color tint) {
    auto cameraPosF = getCameraPositionPrecise();
    // auto cameraPosF = toFloatVec(getCameraPosition());

    constexpr s32 spacing = 0;  // PARAM

    // constexpr s32 lineheight = spacing + fontSize;
    // constexpr s32 menuHeight = lineheight * N_BUTTONS - spacing;  // n-1 fence posts
    // constexpr s32 startHeight = WINDOW_HEIGHT_ACTUAL / 2 - menuHeight / 2;
    // const Font* font = Game::instance().getFont();

    // sorting not required since Draw components don't have transparency
    for (auto const [entityid, entity] : getEntitiesMutable()) {
        if (entity.has<Invisible>()) {
            continue;
        }
        const Transform2D trans = entity.get<Transform2D>();
        const DrawText draw = entity.get<DrawText>();

        Vector2f frameSize = draw.frameSizeTexels.as<f32>() * FPIXELS_PER_TEXEL * VIRTUAL_SCREEN_RATIO * draw.scale;

        // text is drawn at full resolution
        Vector2f dstPosition = {trans.position.x - cameraPosF.x, -1 * trans.position.y + cameraPosF.y};
        dstPosition *= VIRTUAL_SCREEN_RATIO;
        dstPosition += Vector2f(WINDOW_WIDTH_ACTUAL / 2, WINDOW_HEIGHT_ACTUAL / 2);

        // drawing one line:
        // Vector2 textDimensions = MeasureTextEx(DEFAULT_FONT, draw.text, FONT_SIZE, spacing);
        // dstPosition -= Vector2f(textDimensions.x / 2, textDimensions.y);
        // DrawTextEx(DEFAULT_FONT, draw.text, Vector2(dstPosition.x, dstPosition.y), FONT_SIZE, spacing, ColorTint(draw.color, tint));

        // drawing wrapped:
        dstPosition -= frameSize * Vector2f(0.5, 1);
        dstPosition +=
            Vector2f(0, FPIXELS_PER_TILE / 2 * VIRTUAL_SCREEN_RATIO);  // needs half tile offset for some reason; might be an issue with map data
        Rectangle dstRect = Rectangle(dstPosition.x, dstPosition.y, frameSize.x, frameSize.y);
        DrawTextBoxed(DEFAULT_FONT, draw.text.c_str(), dstRect, FONT_SIZE, spacing, true, draw.isCentered, ColorTint(draw.color, tint));
    }
}

void DrawDebugSystem::drawEntities() {
    auto cameraPosF = getCameraPositionPrecise();
    // auto cameraPosF = toFloatVec(getCameraPosition());

    // sorting not required since Draw components don't have transparency
    for (auto const [entityid, entity] : getEntitiesMutable()) {
        const Transform2D trans = entity.get<Transform2D>();
        const DrawDebug draw = entity.get<DrawDebug>();

        auto frameSize = draw.getFrameSizeTexels().as<f32>();
        Vector2f dstSize = {frameSize.x * draw.scale.x * FPIXELS_PER_TEXEL, frameSize.y * draw.scale.y * FPIXELS_PER_TEXEL};

        // subtract size.y so we draw from bottom left instead of top left
        Vector2f dstPosition = {trans.position.x - cameraPosF.x, -1 * trans.position.y + cameraPosF.y - dstSize.y};
        // add halfX to pos to match the origin thingy done w/ sprites
        dstPosition -= {dstSize.x * 0.5f, 0};
        Rectangle dstRect = Rectangle(dstPosition.x, dstPosition.y, dstSize.x, dstSize.y);
        DrawRectangleRec(dstRect, draw.color);
    }
}

void FadeOutSystem::update() {
    f32 dt = System::dt();
    for (auto [entityid, entity] : getEntitiesCopy()) {
        auto& fadeOutComponent = entity.get<FadeOut>();

        fadeOutComponent.secondsRemaining -= dt;
        u8 alpha = fadeOutComponent.getAlpha();

        entity.get<Draw>().setAlpha(alpha);

        if (fadeOutComponent.isDone()) {
            entity.remove<FadeOut>();
        }
    }
}

// // Draw text using font inside rectangle limits
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
