#include "TextRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Gfx/RaylibUtil.h"
#include "Physics/Box.h"
#include "Settings.h"
#include "Util/Vector.h"

#include "raylib.h"

namespace whal {

s32 getFontSize() {
    return 40 * VIRTUAL_SCREEN_RATIO / 4.0f;
}

TextRenderSystem::TextRenderSystem() {
    mFont = new rl::Font();
    *mFont = rl::LoadFontEx(FONT_PATH, getFontSize(), 0, 0);
}

TextRenderSystem::~TextRenderSystem() {
    UnloadFont(*mFont);
    delete mFont;
}

void TextRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    constexpr s32 spacing = 0;         // PARAM
    const Color tint = Colors::White;  // PARAM

    const DrawText draw = eCtx.entity.get<DrawText>();
    const Transform& trans = *eCtx.transform;

    // rotation pivot correction
    Vector2f pivotOffsetScreen = trans.pivotOffset.as<f32>() * Vector2f(1, -1) * VIRTUAL_SCREEN_RATIO;
    Vector2f frameSize = draw.frameSize.as<f32>();

    // scale to full resolution
    frameSize = (frameSize * VIRTUAL_SCREEN_RATIO * trans.scale).absolute();
    Vector2f screenPosition = trans.position * Vector2f(VIRTUAL_SCREEN_RATIO, -VIRTUAL_SCREEN_RATIO);

    // UNUSED
    // {
    //  drawing one line:
    //  Vector2 textDimensions = MeasureTextEx(DEFAULT_FONT, draw.text, FONT_SIZE, spacing);
    //  dstPosition -= Vector2f(textDimensions.x / 2, textDimensions.y);
    //  DrawTextEx(DEFAULT_FONT, draw.text, Vector2(dstPosition.x, dstPosition.y), FONT_SIZE, spacing, ColorTint(draw.color, tint));
    // }

    // TODO needs adjustment based on rotation? Currently not working for multi-line text
    // it's definitely because my DrawTextBoxed function doesn't calculate Y values correctly
    // drawing wrapped:
    // dstPosition -= frameSize * Vector2f(0.5, 0.5);  // original
    screenPosition -= frameSize * Vector2f(0.5, 0.0);  // trying something new
    // needs half tile offset for some reason; might be an issue with map data:
    // dstPosition += Vector2f(0, FPIXELS_PER_TILE / 2 * VIRTUAL_SCREEN_RATIO);

    rl::Rectangle dstRect = rl::Rectangle(screenPosition.x, screenPosition.y, frameSize.x, frameSize.y);

    gfx::RaylibDrawParams params = gfx::RaylibDrawParams{
        .rect = dstRect,
        .origin = rl::Vector2{0, 0},
    };

    gfx::DrawTextBoxed(*mFont, draw.text.c_str(), params, getFontSize(), spacing, draw.isWrapped, draw.isCentered, draw.color * tint, trans.rotation,
                       pivotOffsetScreen, eCtx.colorBuf, trans.scale.absolute().asRL());
}

void TextRenderSystem::addToQueue(gfx::RenderQueue& queue) const {
    for (const auto& [entityid, entity] : getEntities()) {
        const auto draw = entity.get<DrawText>();
        const auto& trans = entity.get<Transform>();

        const auto bb = trans.rotation == 0.0f ? AABB(trans, draw.frameSize / 2, Vector2i()) :
                                                 Box(trans.getRotatedPosition().round(), draw.frameSize / 2, trans.rotation).getBoundingAABB();

        queue.add(gfx::EntityPreRenderInfo{
            .boundingBox = bb,
            .transform = &trans,
            .entity = entity,
        });
    }
}

}  // namespace whal
