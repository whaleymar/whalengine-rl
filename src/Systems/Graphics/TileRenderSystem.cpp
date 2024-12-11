#include "TileRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Tags.h"
#include "Components/Tile.h"
#include "Components/Transform.h"
#include "Gfx/RaylibUtil.h"
#include "Gfx/Texture.h"
#include "Physics/Box.h"

#include "Settings.h"
#include "Sys/System.h"
#include "Util/CameraUtil.h"
#include "rlgl.h"

namespace whal {

static const f32 MIN_CAMERA_MOVE_EPSILON = 0.1;
static bool IS_CULL_RECALC_NEEDED = true;

void TileRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    const auto& sprite = eCtx.entity.get<Sprite>();

    const s32 flipModifier = eCtx.preciseTransform.facing == Facing::Left ? -1 : 1;
    const auto srcRect = rl::Rectangle{sprite.atlasPosition.x, sprite.atlasPosition.y, flipModifier * sprite.frameSize.x, sprite.frameSize.y};

    // simplified version of gfx::getDrawParams that assumes no fancy rotation or scaling
    static const Vector2f size = Vector2f(PIXELS_PER_TILE, PIXELS_PER_TILE) * VIRTUAL_SCREEN_RATIO;
    static const rl::Vector2 origin = (size * Vector2f(0.5, 0.5)).asRL();

    const rl::Rectangle rect = rl::Rectangle{
        (eCtx.preciseTransform.position.x - ctx.cameraPosition.x) * VIRTUAL_SCREEN_RATIO + FWINDOW_WIDTH_RENDER / 2,
        (ctx.cameraPosition.y - eCtx.preciseTransform.position.y) * VIRTUAL_SCREEN_RATIO + FWINDOW_HEIGHT_RENDER / 2,
        size.x,
        size.y,
    };

    gfx::DrawSpriteHDR(ctx.atlas.getTexture(), srcRect, rect, origin, eCtx.preciseTransform.rotationDegrees, sprite.color.asRL(),
                       eCtx.colorBuf.asRL(sprite, ctx.atlas.getSize()));
}

void TileRenderSystem::addToQueue(gfx::RenderQueue& queue) const {
    static Vector2f cameraPosPrev = Vector2f(-909090, 88383);  // unreasonable default so everything calc'd frame 1
    static bool isUpdateNextFrame = false;

    // tiles don't move, so if the camera doesn't move, don't recalculate culling
    const Vector2f cameraPos = getCamera()->get<PrecisePosition>().position;
    const f32 distance = (cameraPos - cameraPosPrev).len();

    if (IS_CULL_RECALC_NEEDED) {
        IS_CULL_RECALC_NEEDED = false;
        cameraPosPrev = cameraPos;
        for (const auto& [entityid, entity] : getEntities()) {
            auto& tile = entity.get<Tile>();
            tile.wasDrawnLastFrame = queue.add(tile.renderInfo);
        }

    } else if (!isUpdateNextFrame && distance < MIN_CAMERA_MOVE_EPSILON) {
        for (const auto& [entityid, entity] : getEntities()) {
            const auto& tile = entity.get<Tile>();
            if (tile.wasDrawnLastFrame) {
                queue.addPrecalculated(tile.renderInfo);
            }
        }
    } else {
        // this only gets updated when we *do* recalculate culling, so repeated tiny camera movements don't mess us up
        // if we updated half the entities last frame, don't overwrite the camera pos. Might cause last frame's draw cache to be wrong.
        if (isUpdateNextFrame) {
            cameraPosPrev = cameraPos;
        }

        // calculate odd numbered entities on odd frames, even numbered on even frames
        isUpdateNextFrame = !isUpdateNextFrame;
        const s32 frameMod2 = Time.getFrame() % 2;
        for (const auto& [entityid, entity] : getEntities()) {
            if ((entityid % 2) == frameMod2) {
                // recalculate culling
                auto& tile = entity.get<Tile>();
                tile.wasDrawnLastFrame = queue.add(tile.renderInfo);
            } else {
                // use last result
                const auto& tile = entity.get<Tile>();
                if (tile.wasDrawnLastFrame) {
                    queue.addPrecalculated(tile.renderInfo);
                }
            }
        }
    }
}

void TileRenderSystem::onAdd(ecs::Entity entity) {
    const auto& trans = entity.get<Transform>();
    const auto bb = AABB(trans, Vector2i(PIXELS_PER_TILE, PIXELS_PER_TILE) / 2, Vector2i());

    entity.get<Tile>().renderInfo = gfx::EntityPreRenderInfo{
        .boundingBox = bb,
        .preciseTransform = gfx::getPreciseTrans(entity, trans),
        .entity = entity,
        .isOccluder = entity.has<BlocksLight>() ? gfx::EntityPreRenderInfo::IsOccluder::Yes : gfx::EntityPreRenderInfo::IsOccluder::No,
    };
}

void TileRenderSystem::onEvent(evt::Restart, bool) {
    IS_CULL_RECALC_NEEDED = true;
}

}  // namespace whal
