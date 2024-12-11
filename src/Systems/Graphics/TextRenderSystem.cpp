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

static const s32 FONT_SIZE = 40 * VIRTUAL_SCREEN_RATIO / 4.0f;

TextRenderSystem::TextRenderSystem() {
    mFont = new rl::Font();
    *mFont = rl::LoadFontEx(FONT_PATH, FONT_SIZE, 0, 0);
}

TextRenderSystem::~TextRenderSystem() {
    UnloadFont(*mFont);
    delete mFont;
}

void TextRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    constexpr s32 spacing = 0;         // PARAM
    const Color tint = Colors::White;  // PARAM

    const DrawText draw = eCtx.entity.get<DrawText>();
    const auto& trans = eCtx.preciseTransform;

    // rotation pivot correction
    Vector2f pivotOffsetScreen = trans.pivotOffset.as<f32>() * Vector2f(1, -1) * VIRTUAL_SCREEN_RATIO;
    Vector2f frameSize = draw.frameSize.as<f32>();

    // scale to full resolution
    frameSize = frameSize * VIRTUAL_SCREEN_RATIO * trans.scale;
    Vector2f dstPosition = {trans.position.x - ctx.cameraPosition.x, -1 * trans.position.y + ctx.cameraPosition.y};
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
    // dstPosition -= frameSize * Vector2f(0.5, 0.5);  // original
    dstPosition -= frameSize * Vector2f(0.5, 0.0);  // trying something new
    // needs half tile offset for some reason; might be an issue with map data:
    // dstPosition += Vector2f(0, FPIXELS_PER_TILE / 2 * VIRTUAL_SCREEN_RATIO);

    rl::Rectangle dstRect = rl::Rectangle(dstPosition.x, dstPosition.y, frameSize.x, frameSize.y);

    gfx::RaylibDrawParams params = gfx::RaylibDrawParams{
        .rect = dstRect,
        .origin = rl::Vector2{0, 0},
        .position = dstPosition.asRL(),
    };

    gfx::DrawTextBoxed(*mFont, draw.text.c_str(), params, FONT_SIZE, spacing, true, draw.isCentered, draw.color * tint, trans.rotationDegrees,
                       pivotOffsetScreen, eCtx.colorBuf);
}

void TextRenderSystem::addToQueue(gfx::RenderQueue& queue) const {
    for (const auto& [entityid, entity] : getEntities()) {
        const auto draw = entity.get<DrawText>();
        const auto trans = entity.get<Transform>();

        const auto bb = trans.rotationDegrees == 0.0f ? AABB(trans, draw.frameSize / 2, Vector2i()) :
                                                        Box(trans.getRotatedPosition(), draw.frameSize / 2, trans.rotationDegrees).getBoundingAABB();

        queue.add(gfx::EntityPreRenderInfo{
            .boundingBox = bb,
            .preciseTransform = gfx::getPreciseTrans(entity, trans),
            .entity = entity,
        });
    }
}

}  // namespace whal
