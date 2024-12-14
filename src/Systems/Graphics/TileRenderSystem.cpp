#include "TileRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/TileMapLayer.h"
#include "Components/Transform.h"
#include "Gfx/RaylibUtil.h"
#include "Gfx/Texture.h"
#include "Map/Tiled.h"
#include "Physics/Box.h"

#include "Settings.h"
#include "Util/CameraUtil.h"
#include "rlgl.h"

namespace whal {

static std::pair<f32, Facing> getOrientation(TileInfo tile);

void TileRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    ecs::Entity layerEntity = eCtx.entity;
    const TileMapLayer& layer = layerEntity.get<TileMapLayer>();
    const Vector2f tileSize = Vector2f(PIXELS_PER_TILE, PIXELS_PER_TILE) * VIRTUAL_SCREEN_RATIO * eCtx.preciseTransform.scale;
    const rl::Vector2 origin = (tileSize * Vector2f(0.5, 0.5)).asRL();

    // TODO render quadrant or individual tile based on Entity ID
    for (s32 x = 0; x < layer.tilemap->widthTiles; x++) {
        for (s32 y = 0; y < layer.tilemap->heightTiles; y++) {
            const s32 ix = layer.tilemap->widthTiles * y + x;
            const TileInfo tile = getTile(layer.ids[ix]);

            if (tile.gid == 0) {
                continue;  // empty tile
            }

            Sprite sprite;
            auto it = layer.tilemap->spriteCache.find(tile.gid);
            if (it == layer.tilemap->spriteCache.end()) {
                sprite = getTileSprite(*layer.tilemap.get(), tile.gid).value();
                layer.tilemap->spriteCache.insert({tile.gid, sprite});
            } else {
                sprite = it->second;
            }

            const auto orient = getOrientation(tile);
            const auto srcRect = rl::Rectangle{
                sprite.atlasPosition.x,
                sprite.atlasPosition.y,
                (orient.second == Facing::Left ? -1 : 1) * sprite.frameSize.x,
                sprite.frameSize.y,
            };

            Vector2f worldPosition = Vector2f(x * PIXELS_PER_TILE, -y * PIXELS_PER_TILE) + eCtx.preciseTransform.position;

            const rl::Rectangle rect = rl::Rectangle{
                (worldPosition.x - ctx.cameraPosition.x) * VIRTUAL_SCREEN_RATIO + FWINDOW_WIDTH_RENDER / 2,
                (ctx.cameraPosition.y - worldPosition.y) * VIRTUAL_SCREEN_RATIO + FWINDOW_HEIGHT_RENDER / 2,
                tileSize.x,
                tileSize.y,
            };

            auto meta = eCtx.colorBuf;
            meta.isOccluder = layer.collisionMask[ix];
            gfx::DrawSpriteHDR(ctx.atlas.getTexture(), srcRect, rect, origin, orient.first + eCtx.preciseTransform.rotationDegrees,
                               sprite.color.asRL(), meta.asRL(sprite, ctx.atlas.getSize()));
        }
    }
}

void TileRenderSystem::addToQueue(gfx::RenderQueue& queue) const {
    // TODO add quadrants or individual tiles based on Y sort flag
    for (const auto& [entityid, entity] : getEntities()) {
        const auto& trans = entity.get<Transform>();
        const auto& tml = entity.get<TileMapLayer>();

        const Vector2i half = tml.sizeTiles * Vector2i(PIXELS_PER_TILE / 2, PIXELS_PER_TILE / 2);
        const Vector2i center = trans.position + half * Vector2i(1, -1);
        const auto bb = AABB(center, half);

        queue.add(gfx::EntityPreRenderInfo{
            .boundingBox = bb,
            .preciseTransform = gfx::getPreciseTrans(entity, trans),
            .entity = entity,
            .isOccluder = gfx::EntityPreRenderInfo::IsOccluder::No,
        });
    }
}

std::pair<f32, Facing> getOrientation(TileInfo tile) {
    TileInfo originalTile = tile;
    Facing facing = Facing::Right;
    f32 rotation = 0;

    // the rotate flag technically means diagonal flipping or something idk it's some jank
    if (tile.isRotate) {
        if (tile.isFlipY) {
            tile.isFlipH = !tile.isFlipH;
        }
        if (!tile.isFlipH) {
            tile.isFlipY = !tile.isFlipY;
        } else if (!tile.isFlipY) {
            tile.isFlipH = false;
        }
    }

    if (originalTile.isRotate && originalTile.isFlipY && originalTile.isFlipH) {
        facing = Facing::Left;
        // trans.rotationDegrees = 180;
    } else if (tile.isFlipH && !tile.isFlipY) {
        facing = Facing::Left;
    } else if (tile.isFlipY && !tile.isFlipH) {
        rotation = 180;
        facing = Facing::Left;
    } else if (tile.isFlipH && tile.isFlipY) {
        rotation = 180;
    }

    if (tile.isRotate) {
        rotation += 90;
    }

    return {rotation, facing};
}

}  // namespace whal
