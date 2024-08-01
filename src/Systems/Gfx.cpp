#include "Gfx.h"

#include <algorithm>
#include <raylib.h>

#include "Components/Tags.h"
#include "Gfx/Depth.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Physics/Shapes.h"
#include "Settings.h"

#include "Systems/CollisionManager.h"
#include "Systems/TagTrackers.h"
#include "Util/Print.h"
#include "Util/Vector.h"

#include "Components/Draw.h"
#include "Components/Transform.h"

namespace whal {

static void DrawTextBoxed(Font font, const char* text, Rectangle rec, float fontSize, float spacing, bool wordWrap, bool center, Color tint);
static void DrawTextBoxedSelectable(Font font, const char* text, Rectangle rec, float fontSize, float spacing, bool wordWrap, bool center, Color tint,
                                    int selectStart, int selectLength, Color selectTint, Color selectBackTint);

static Font DEFAULT_FONT;
static const s32 FONT_SIZE = 40 * VIRTUAL_SCREEN_RATIO / 4.0f;

static const std::array<TextureID, 3> S_LAYER_TEXTURES = {
    TextureID::LayerNormal,
    TextureID::LayerBloom,
    TextureID::LayerGlow,
};

static Vector2 toScreenCoord(Vector2i worldCoord, Vector2i cameraPos) {
    return Vector2(worldCoord.x - cameraPos.x, cameraPos.y - worldCoord.y);
}

GfxSystem::Layer& GfxSystem::getLayer(TextureID texId) {
    switch (texId) {
    case TextureID::LayerNormal:
        return mLayerNormal;
    case TextureID::LayerBloom:
        return mLayerBloom;
    case TextureID::LayerGlow:
        return mLayerGlow;
    default:
        print("GfxSystem does not handle layer for TextureID: ", static_cast<s32>(texId));
        return mLayerNormal;
    }
}

// sorts new entities
void GfxSystem::Layer::update() {
    std::sort(toSort.begin(), toSort.end(), &isBelow);

    auto it = sorted.before_begin();
    auto current = sorted.begin();
    auto insertIt = toSort.begin();

    // insert new elements in sorted order:
    while (current != sorted.end()) {
        while (insertIt != toSort.end() && isBelow(*insertIt, *current)) {
            it = sorted.insert_after(it, *insertIt);
            ++insertIt;
        }
        ++it;
        ++current;
    }
    // insert remaining elements in vec:
    while (insertIt != toSort.end()) {
        it = sorted.insert_after(it, *insertIt);
        ++insertIt;
    }

    toSort.clear();

    // reset iterator
    iter = sorted.begin();
}

void GfxSystem::onAdd(const ecs::Entity entity) {
    const auto draw = entity.get<Draw>();
    const DrawInfo drawInfo(entity, depthToFloat(draw.getDepth()), draw.getDepth(), static_cast<s16>(draw.getShader()));
    getLayer(draw.getTexLayer()).toSort.push_back(drawInfo);
}

void GfxSystem::onRemove(const ecs::Entity entity) {
    auto pred = [entity](const DrawInfo& drawInfo) { return drawInfo.entity.id() == entity.id(); };
    auto& layer = getLayer(entity.get<Draw>().getTexLayer());
    auto numRemoved = layer.sorted.remove_if(pred);
    if (numRemoved == 0) {
        auto it = std::find_if(layer.toSort.begin(), layer.toSort.end(), pred);
        assert(it != layer.toSort.end() && "tried to remove entity from GfxSystem, but couldn't find it in layer.sorted or layer.toSort collections");
        layer.toSort.erase(it);
    }
}

// returns true if `first` should be drawn before `seccond`
// based on depth, then shader
bool GfxSystem::isBelow(const DrawInfo& first, const DrawInfo& second) {
    return !(first.depth > second.depth || (first.depth == second.depth && first.shaderIx >= second.shaderIx));
}

static void drawTextureFlipped(const Texture& tex) {
    DrawTextureRec(tex, Rectangle(0, 0, tex.width, -tex.height), Vector2(0, 0), WHITE);
}

// this is really messy and will be a pain to add more layers to it
void GfxSystem::drawEntities(Camera2D worldCamera) {
    // update layers with new entities
    for (auto texID : S_LAYER_TEXTURES) {
        getLayer(texID).update();
    }

    bool isFirstDrawToMain = true;
    bool isFirstDrawToOcclusion = true;

    // draw one layer at a time to minimize FBO swaps
    while (true) {
        Depth currentDepth = Depth::Debug;
        for (size_t i = 0; i < S_LAYER_TEXTURES.size(); i++) {
            const auto& layer = getLayer(S_LAYER_TEXTURES[i]);
            if (layer.iter == layer.sorted.end() || layer.iter->depthId == currentDepth) {
                continue;
            }

            if (layer.iter->depth < depthToFloat(currentDepth)) {
                currentDepth = layer.iter->depthId;
            }
        }

        std::bitset<S_LAYER_TEXTURES.size()> drawMask;
        for (size_t i = 0; i < S_LAYER_TEXTURES.size(); i++) {
            const auto texID = S_LAYER_TEXTURES[i];
            auto& layer = getLayer(texID);

            if (layer.iter != layer.sorted.end() && currentDepth == layer.iter->depthId) {
                drawMask.set(i);
                BeginTextureMode(TextureManager::getRenderTexture(texID));
                ClearBackground(Colors::Clear);
                BeginMode2D(worldCamera);
                layer.iter = drawEntities(layer, layer.iter);
                EndMode2D();
                EndTextureMode();

                if (currentDepth == Depth::Level) {
                    // draw to occlusion mask
                    BeginTextureMode(TextureManager::getRenderTexture(TextureID::Occlusion));
                    if (isFirstDrawToOcclusion) {
                        ClearBackground(Colors::Clear);
                        isFirstDrawToOcclusion = false;
                    }
                    BeginBlendMode(BLEND_ADDITIVE);
                    drawTextureFlipped(TextureManager::getRenderTexture(texID).texture);
                    EndBlendMode();
                    EndTextureMode();
                }
            }
        }

        BeginTextureMode(TextureManager::getRenderTexture(TextureID::Main));
        if (isFirstDrawToMain) {
            ClearBackground(Colors::Clear);
            isFirstDrawToMain = false;
        }

        bool isDone = true;
        for (size_t i = 0; i < S_LAYER_TEXTURES.size(); i++) {
            const auto& layer = getLayer(S_LAYER_TEXTURES[i]);
            if (!isDone || layer.iter != layer.sorted.end()) {
                isDone = false;
            }

            if (!drawMask[i]) {
                continue;
            }
            if (layer.shader == Shaders::Default) {
                drawTextureFlipped(TextureManager::getRenderTexture(S_LAYER_TEXTURES[i]).texture);
            } else {
                ScopedShader shaderScope = ShaderManager::activateScoped(layer.shader);
                drawTextureFlipped(TextureManager::getRenderTexture(S_LAYER_TEXTURES[i]).texture);
            }
        }

        if (isDone) {
            break;
        } else {
            EndTextureMode();
        }
    }

    // TextureManager::instance().drawLightingTexture();

#ifndef NDEBUG
    BeginMode2D(worldCamera);
    if (System::input.isOn(InputType::DEBUG)) {
        System::world.getSystem<DrawDebugSystem>()->drawEntities();
        drawColliders();
    }
    EndMode2D();
#endif

    EndTextureMode();
}

// draw entities in layer starting at the passed iterator. Stops when the next entity has a new depth value
std::forward_list<GfxSystem::DrawInfo>::iterator GfxSystem::drawEntities(Layer& layer, std::forward_list<DrawInfo>::iterator startIt) {
    auto cameraPosF = getCameraPositionPrecise();
    // auto cameraPosF = toFloatVec(getCameraPosition());
    const Texture2D& spriteTexture = TextureManager::instance().getTextureAtlas(TEXNAME_SPRITE).getTexture();

    Shaders prevShader = static_cast<Shaders>(layer.sorted.begin()->shaderIx);
    ShaderManager::activate(prevShader);
    std::forward_list<DrawInfo>::iterator it;
    for (it = startIt; it != layer.sorted.end() && it->depthId == startIt->depthId; ++it) {
        const auto drawInfo = *it;
        if (drawInfo.entity.has<Invisible>()) {
            continue;
        }
        Shaders newShader = static_cast<Shaders>(drawInfo.shaderIx);
        if (newShader != prevShader) {
            prevShader = newShader;
            EndShaderMode();
            ShaderManager::activate(newShader);
        }
        drawEntity(drawInfo.entity, spriteTexture, cameraPosF);
    }
    EndShaderMode();
    return it;
}

void GfxSystem::drawEntity(ecs::Entity entity, const Texture2D& spriteTexture, const Vector2f cameraPosF) {
    const Transform2D trans = entity.get<Transform2D>();
    Vector2f posF = trans.position.as<f32>();
    Draw draw = entity.get<Draw>();
    const Vector2i cameraPos = cameraPosF.round();
    const AABB cameraViewBox(cameraPos, {WINDOW_WIDTH_PIXELS / 2, WINDOW_HEIGHT_PIXELS / 2});

    switch (draw.getTag()) {
    case Draw::DrawTag::Rect: {
        const DrawRect rect = draw.getRect();
        auto frameSize = rect.getFrameSizeTexels().as<f32>();
        Vector2f dstSize = {frameSize.x * rect.scale.x * FPIXELS_PER_TEXEL, frameSize.y * rect.scale.y * FPIXELS_PER_TEXEL};

        // skip if entity is off screen
        // use generous 2x'd half len so we don't have to worry about rotations
        if (AABB drawBox = AABB(trans.position, dstSize.as<s32>()); !cameraViewBox.isOverlapping(drawBox)) {
            return;
        }

        // subtract size.y so we draw from bottom left instead of top left
        Vector2f dstPosition = {trans.position.x - cameraPosF.x, -1 * trans.position.y + cameraPosF.y - dstSize.y};
        // add halfX to pos to match the origin thingy done w/ sprites
        dstPosition -= {dstSize.x * 0.5f, 0};
        Rectangle dstRect = Rectangle(dstPosition.x, dstPosition.y, dstSize.x, dstSize.y);
        DrawRectangleRec(dstRect, rect.color);
        break;
    }
    case Draw::DrawTag::Sprite: {
        const Sprite sprite = draw.getSprite();
        const Vector2i frameSize = sprite.getFrameSizeTexels();
        const s32 flipModifier = trans.facing == Facing::Left ? -1 : 1;
        const Rectangle srcRect = Rectangle(sprite.atlasPositionTexels.x, sprite.atlasPositionTexels.y, flipModifier * frameSize.x, frameSize.y);

        Vector2f dstSize = {frameSize.x * sprite.scale.x * FPIXELS_PER_TEXEL, frameSize.y * sprite.scale.y * FPIXELS_PER_TEXEL};

        // skip if entity is off screen
        // use generous 2x'd half len so we don't have to worry about rotations
        if (AABB drawBox = AABB(trans.position, dstSize.as<s32>()); !cameraViewBox.isOverlapping(drawBox)) {
            return;
        }

        Vector2f dstPosition = {posF.x - cameraPosF.x, -1.0f * posF.y + cameraPosF.y};

        // rotate about center or transform
        Vector2f origin = sprite.isRotateAboutCenter ? dstSize * Vector2f(0.5, 0.5) : Vector2f(dstSize.x * 0.5, dstSize.y);
        if (sprite.isRotateAboutCenter) {
            dstPosition -= Vector2f(0, dstSize.y / 2.0f);
        }

        Rectangle dstRect = Rectangle(dstPosition.x, dstPosition.y, dstSize.x, dstSize.y);
        DrawTexturePro(spriteTexture, srcRect, dstRect, {origin.x, origin.y}, trans.rotationDegrees, sprite.color);
        break;
    }
    case Draw::DrawTag::BezierQuad: {
        const DrawBezierQuad bezier = draw.getBezierQuad();

        DrawSplineSegmentBezierQuadratic(toScreenCoord(trans.position, cameraPos),
                                         toScreenCoord(trans.position + bezier.controlPointOffset, cameraPos),
                                         toScreenCoord(trans.position + bezier.endPointOffset, cameraPos), bezier.thickness, bezier.color);
        break;
    }
    case Draw::DrawTag::Line: {
        const DrawStraightLine line = draw.getLine();

        Vector2i startPos;
        Vector2i endPos;
        if (line.isRotateAboutCenter) {
            Vector2f halfLine = angleToUnit(trans.rotationDegrees) * static_cast<f32>(line.length) * 0.5f;
            startPos = (trans.position.as<f32>() - halfLine).round();
            endPos = (trans.position.as<f32>() + halfLine).round();

        } else {
            startPos = trans.position;
            endPos = trans.position + (angleToUnit(trans.rotationDegrees) * (f32)line.length).round();
        }

        DrawLineEx(toScreenCoord(startPos, cameraPos), toScreenCoord(endPos, cameraPos), line.thickness, line.color);
        break;
    }
    }
}

DrawTextSystem::DrawTextSystem() {
    DEFAULT_FONT = LoadFontEx(FONT_PATH, FONT_SIZE, 0, 0);
}

void DrawTextSystem::drawEntities(Color tint) {
    auto cameraPosF = getCameraPositionPrecise();
    constexpr s32 spacing = 0;  // PARAM

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

void ColorLerpSystem::update() {
    f32 dt = System::dt();
    for (auto [entityid, entity] : getEntitiesCopy()) {
        auto colorLerp = entity.get<ColorLerp>();
        colorLerp.tick(dt);
        entity.get<Draw>().setColor(colorLerp.getColor());
        if (colorLerp.isDone()) {
            entity.remove<ColorLerp>();
        } else {
            entity.set(colorLerp);
        }
    }
}

void ScaleLerpSystem::update() {
    f32 dt = System::dt();
    for (auto [entityid, entity] : getEntitiesCopy()) {
        auto scaleLerp = entity.get<ScaleLerp>();
        scaleLerp.tick(dt);
        entity.get<Draw>().setScale(scaleLerp.getScale());
        if (scaleLerp.isDone()) {
            entity.remove<ScaleLerp>();
        } else {
            entity.set(scaleLerp);
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
