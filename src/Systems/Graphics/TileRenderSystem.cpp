#include "TileRenderSystem.h"

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Map.h"
#include "Components/Transform.h"
#include "Gfx/RaylibUtil.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Map/Tiled.h"
#include "Physics/Box.h"

#include "Settings.h"
#include "Util/CameraUtil.h"
#include "rlgl.h"

namespace whal {

static std::pair<f32, Facing> getOrientation(TileInfo tile);
static void buildYsortList(ecs::Entity e, const TileMapLayer& tml);
static std::unordered_map<ecs::Entity, std::vector<Vector2i>, ecs::EntityHash> S_YSORT_COORD_LUT;
static std::unordered_map<ecs::Entity, std::vector<gfx::EntityPreRenderInfo>, ecs::EntityHash> S_YSORT_RENDERINFO_LUT;

void TileRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    ecs::Entity layerEntity = eCtx.entity;
    const TileMapLayer& layer = layerEntity.get<TileMapLayer>();
    const Vector2f tileSize = Vector2f(PIXELS_PER_TILE, PIXELS_PER_TILE) * VIRTUAL_SCREEN_RATIO * eCtx.transform.scale;
    const rl::Vector2 origin = (tileSize * Vector2f(0.5, 0.5)).asRL();

    // this shader could be slightly faster & more ergonomic if I make it a Shader class
    if (layer.overlay) {
        // note: overlays will be slow for Y sorted layers
        auto shader = ShaderManager::get(Shaders::Overlay);
        rl::BeginShaderMode(shader);
        auto overlayLoc = rl::GetShaderLocation(shader, "_Overlay");
        rl::SetShaderValueTexture(shader, overlayLoc, *layer.overlay);
        auto scaleLoc = rl::GetShaderLocation(shader, "_Scale");
        rl::Vector2 scale =
            (Vector2f(1.0f / VIRTUAL_SCREEN_RATIO, 1.0f / VIRTUAL_SCREEN_RATIO) / Vector2f(layer.overlay->width, layer.overlay->height)).asRL();
        rl::SetShaderValue(shader, scaleLoc, &scale, rl::SHADER_UNIFORM_VEC2);
    }

    const auto drawTile = [&](s32 x, s32 y, s32 ix, TileInfo tile) {
        // RESEARCH this lookup is SLOW and makes me want to ditch the STL
        // const Sprite& sprite = layer.tilemap->spriteCache[tile.gid];
        const TileRenderInfo& renderInfo = layer.tilemap->spriteCache.get(tile.gid);
        const auto srcRect = rl::Rectangle{
            renderInfo.sprite.atlasPosition.x,
            renderInfo.sprite.atlasPosition.y,
            (renderInfo.orient.second == Facing::Left ? -1 : 1) * renderInfo.sprite.frameSize.x,
            renderInfo.sprite.frameSize.y,
        };

        const Vector2f worldPosition = Vector2f(x * PIXELS_PER_TILE, -y * PIXELS_PER_TILE) + eCtx.transform.position;

        const rl::Rectangle rect = rl::Rectangle{
            worldPosition.x * VIRTUAL_SCREEN_RATIO,
            -worldPosition.y * VIRTUAL_SCREEN_RATIO,
            tileSize.x,
            tileSize.y,
        };

        auto meta = eCtx.colorBuf;
        meta.isOccluder = renderInfo.isOccluder;
        gfx::DrawSpriteHDR(ctx.atlas.getTexture(), srcRect, rect, origin, renderInfo.orient.first + eCtx.transform.rotation,
                           renderInfo.sprite.color.asRL(), meta.asRL(renderInfo.sprite, ctx.atlas.getSize()));
    };

    if (layer.isYSorted) {
        const s32 ySortIx = eCtx.internal;
        const Vector2i coord = S_YSORT_COORD_LUT[eCtx.entity][ySortIx];
        const s32 ix = layer.tilemap->widthTiles * coord.y + coord.x;
        drawTile(coord.x, coord.y, ix, getTile(layer.ids[ix]));

    } else {
        for (s32 x = 0; x < layer.tilemap->widthTiles; x++) {
            for (s32 y = 0; y < layer.tilemap->heightTiles; y++) {
                const s32 ix = layer.tilemap->widthTiles * y + x;
                const TileInfo tile = getTile(layer.ids[ix]);

                if (tile.gid == 0) {
                    continue;  // empty tile
                }

                drawTile(x, y, ix, tile);
            }
        }
    }
}

void TileRenderSystem::addToQueue(gfx::RenderQueue& queue) const {
    for (const auto& [entityid, entity] : getEntities()) {
        const auto& trans = entity.get<Transform>();
        const auto& tml = entity.get<TileMapLayer>();

        const Vector2i half = tml.sizeTiles * Vector2i(PIXELS_PER_TILE / 2, PIXELS_PER_TILE / 2);
        const Vector2i center = trans.positionPx + half * Vector2i(1, -1);
        const auto bb = AABB(center, half);

        if (tml.isYSorted) {
            buildYsortList(entity, tml);
            for (const auto& renderInfo : S_YSORT_RENDERINFO_LUT[entity]) {
                // TODO addSorted method?
                queue.add(renderInfo);
            }
        } else {
            queue.add(gfx::EntityPreRenderInfo{
                .boundingBox = bb,
                .transform = trans,
                .entity = entity,
                .isOccluder = gfx::EntityPreRenderInfo::IsOccluder::No,
            });
        }
    }
}

void TileRenderSystem::onAdd(ecs::Entity e) {
    // make sure all the tile sprites for this layer are in the cache
    const TileMapLayer& layer = e.get<TileMapLayer>();
    for (s32 x = 0; x < layer.tilemap->widthTiles; x++) {
        for (s32 y = 0; y < layer.tilemap->heightTiles; y++) {
            const s32 ix = layer.tilemap->widthTiles * y + x;
            const TileInfo tile = getTile(layer.ids[ix]);

            if (tile.gid == 0) {
                continue;  // empty tile
            }

            if (!layer.tilemap->spriteCache.contains(tile.gid)) {
                const auto sprite = getTileSprite(*layer.tilemap.get(), tile.gid).value();
                const auto orient = getOrientation(tile);
                layer.tilemap->spriteCache.insert({tile.gid, TileRenderInfo{
                                                                 .sprite = sprite,
                                                                 .orient = orient,
                                                                 .isOccluder = layer.collisionMask[ix],
                                                             }});
            }
        }
    }
}

void TileRenderSystem::onRemove(ecs::Entity e) {
    // erase from cache
    S_YSORT_RENDERINFO_LUT.erase(e);
    S_YSORT_COORD_LUT.erase(e);
    TileMapLayer& layer = e.get<TileMapLayer>();
    if (layer.overlay) {
        rl::UnloadTexture(*layer.overlay);
        layer.overlay = std::nullopt;
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

void buildYsortList(ecs::Entity e, const TileMapLayer& tml) {
    // by assuming tiles don't move in world space, we can cache the result from the first frame this entity was drawn.
    if (S_YSORT_COORD_LUT.contains(e)) {
        return;
    }

    S_YSORT_COORD_LUT[e] = {};
    S_YSORT_RENDERINFO_LUT[e] = {};
    const Transform parentTrans = e.get<Transform>();
    const Vector2i halflen(PIXELS_PER_TILE / 2, PIXELS_PER_TILE / 2);
    s32 lut_ix = 0;

    for (s32 x = 0; x < tml.tilemap->widthTiles; x++) {
        for (s32 y = 0; y < tml.tilemap->heightTiles; y++) {
            const s32 ix = tml.tilemap->widthTiles * y + x;
            const TileInfo tile = getTile(tml.ids[ix]);

            if (tile.gid == 0) {
                continue;  // empty tile
            }

            S_YSORT_COORD_LUT[e].push_back(Vector2i(x, y));
            Vector2f worldPosition = Vector2f(x * PIXELS_PER_TILE, -y * PIXELS_PER_TILE) + parentTrans.position;

            // Value of IsOccluder can be Yes or No; doesn't matter because it's calculated at draw time.
            // The important part is that it's not Unchecked because the RenderQueue will waste time checking.
            const auto ri = gfx::EntityPreRenderInfo{
                .boundingBox = AABB(worldPosition.round(), halflen),
                .transform = parentTrans,
                .entity = e,
                .isOccluder = gfx::EntityPreRenderInfo::IsOccluder::No,
                .internal = lut_ix,
            };
            S_YSORT_RENDERINFO_LUT[e].push_back(ri);

            lut_ix++;
        }
    }
}

}  // namespace whal
