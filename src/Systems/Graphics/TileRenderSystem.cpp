#include "TileRenderSystem.h"
#include <cstring>

#include "Common.h"
#include "Components/Draw.h"
#include "Components/Map.h"
#include "Components/Transform.h"
#include "Gfx/RaylibUtil.h"
#include "Gfx/Shader.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Map/Tiled.h"

#include "Settings.h"
#include "Sys/System.h"
#include "Util/CameraUtil.h"
#include "raylib.h"
#include "rlgl.h"

namespace whal {

struct YsortedTileInfo {
    gfx::EntityPreRenderInfo renderInfo;
    const TileRenderInfo* pTileRenderInfo;
    Vector2i coord;
};

static void buildYsortList(ecs::Entity e, const TileMapLayer& tml);
static std::unordered_map<ecs::Entity, std::vector<TileInstance>, ecs::EntityHash> S_TILES_NORMAL;
static std::unordered_map<ecs::Entity, std::vector<YsortedTileInfo>, ecs::EntityHash> S_TILES_YSORTED;

// NOTE: assumptions made when drawing tiles:
/*

- tiles don't move independently of their TileMapLayer
- tile IDs can't be swapped once added to this system (can be circumvented if I add a cache invalidation mechanism, or by deactivating/reactivating)

*/

// slightly optimized version of DrawSpriteHDR
// RESEARCH another optimization I could do (if I move this whole function into rlgl.h) is to batch all 4 vertices together and move them with a
// single memcpy.
//      Moving this to rlgl means I'd have to re-convert `source` to an rl::Rectangle and pass the tile dims as an argument.
static inline void DrawTileHDR(float invTexWidth, float invTexHeight, rl::Vector2 source, rl::Rectangle dest, rl::Vector2 origin, float rotation,
                               rl::Vector4 hdrColor, rl::Vector3 packedCBI, float z, bool flipX) {
    // Only calculate rotation if needed
    if (rotation == 0.0f) {
        rl::rlBegin(RL_QUADS);
        rl::rlCheckQuadBatch();

        rl::rlColor4f(hdrColor.x, hdrColor.y, hdrColor.z, hdrColor.w);
        rl::rlSetNormals(packedCBI);

        const float texLeft = flipX ? (source.x + FPIXELS_PER_TILE) * invTexWidth : source.x * invTexWidth;
        const float texRight = flipX ? source.x * invTexWidth : (source.x + FPIXELS_PER_TILE) * invTexWidth;
        const float texTop = source.y * invTexHeight;
        const float texBottom = (source.y + FPIXELS_PER_TILE) * invTexHeight;
        const float x = dest.x - origin.x;
        const float y = dest.y - origin.y;

        // Top-left corner for texture and quad
        rl::rlTexCoord2f(texLeft, texTop);
        rl::rlVertex2fNoBatchCheck((float[]){x, y, z});

        // Bottom-left corner for texture and quad
        rl::rlTexCoord2f(texLeft, texBottom);
        rl::rlVertex2fNoBatchCheck((float[]){x, y + dest.height, z});

        // Bottom-right corner for texture and quad
        rl::rlTexCoord2f(texRight, texBottom);
        rl::rlVertex2fNoBatchCheck((float[]){x + dest.width, y + dest.height, z});

        // Top-right corner for texture and quad
        rl::rlTexCoord2f(texRight, texTop);
        rl::rlVertex2fNoBatchCheck((float[]){x + dest.width, y, z});

        // rl::rlEnd(); // just increments depth
    } else {
        float sinRotation = sinf(rotation * DEG2RAD);
        float cosRotation = cosf(rotation * DEG2RAD);
        float x = dest.x;
        float y = dest.y;
        float dx = -origin.x;
        float dy = -origin.y;

        rl::Vector2 topLeft;
        rl::Vector2 topRight;
        rl::Vector2 bottomLeft;
        rl::Vector2 bottomRight;

        topLeft.x = x + dx * cosRotation - dy * sinRotation;
        topLeft.y = y + dx * sinRotation + dy * cosRotation;

        topRight.x = x + (dx + dest.width) * cosRotation - dy * sinRotation;
        topRight.y = y + (dx + dest.width) * sinRotation + dy * cosRotation;

        bottomLeft.x = x + dx * cosRotation - (dy + dest.height) * sinRotation;
        bottomLeft.y = y + dx * sinRotation + (dy + dest.height) * cosRotation;

        bottomRight.x = x + (dx + dest.width) * cosRotation - (dy + dest.height) * sinRotation;
        bottomRight.y = y + (dx + dest.width) * sinRotation + (dy + dest.height) * cosRotation;
        rl::rlBegin(RL_QUADS);

        rl::rlColor4f(hdrColor.x, hdrColor.y, hdrColor.z, hdrColor.w);
        rl::rlSetNormals(packedCBI);

        const float texLeft = flipX ? (source.x + FPIXELS_PER_TILE) * invTexWidth : source.x * invTexWidth;
        const float texRight = flipX ? source.x * invTexWidth : (source.x + FPIXELS_PER_TILE) * invTexWidth;
        const float texTop = source.y * invTexHeight;
        const float texBottom = (source.y + FPIXELS_PER_TILE) * invTexHeight;

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
    }
}

// slightly faster version of DrawMetaData::asRL
rl::Vector3 GetTileMetaFlags(const Sprite& sprite, u8 depth, bool isUI, f32 invTexWidth, f32 invTexHeight) {
    u32 packed = static_cast<u32>(depth);

    // if (isOccluder) {
    //     packed |= (1 << 8);
    // }

    if (isUI) {
        packed |= (1 << 9);
    }

    f32 maskOffsetUVX = 0.0f;
    f32 maskOffsetUVY = 0.0f;
    if (!sprite.maskPosRelative.isZero()) {
        // might want to set a flag in the CBI? Idk i guess i can just check if these values are zero
        maskOffsetUVX = sprite.maskPosRelative.x * invTexWidth;   // x offset
        maskOffsetUVY = sprite.maskPosRelative.y * invTexHeight;  // y offset

        packed |= (1 << 10);  // set flag so we know there's a mask
    }

    if (sprite.isFlagSet(Sprite::Silhouette)) {
        packed |= (1 << 11);
    }

    if (sprite.isFlagSet(Sprite::MaskBlendAdditive)) {
        packed |= (1 << 12);
    }

    f32 x;
    memcpy(&x, &packed, sizeof(f32));
    return {x, maskOffsetUVX, maskOffsetUVY};
}

void TileRenderSystem::draw(const gfx::EntityRenderInfo& eCtx, const gfx::RenderContext& ctx) const {
    const f32 tileSizeX = math::abs(FPIXELS_PER_TILE * VIRTUAL_SCREEN_RATIO * eCtx.transform->scale.x);
    const f32 tileSizeY = math::abs(FPIXELS_PER_TILE * VIRTUAL_SCREEN_RATIO * eCtx.transform->scale.y);
    const rl::Vector2 origin = rl::Vector2{tileSizeX * 0.5f, tileSizeY * 0.5f};

    const f32 invTexSizeX = 1.0f / static_cast<f32>(ctx.atlas.getTexture().width);
    const f32 invTexSizeY = 1.0f / static_cast<f32>(ctx.atlas.getTexture().height);
    const f32 layerPositionX = eCtx.transform->position.x;
    const f32 layerPositionY = eCtx.transform->position.y + eCtx.transform->floatHeight * FLOAT_HEIGHT_MULT;

    // if internal is nonzero, we have a ysorted tile
    if (eCtx.internal) {
        // just drawing one tile
        rl::rlSetTexture(ctx.atlas.getTexture().id);
        const YsortedTileInfo* tile = static_cast<YsortedTileInfo*>(eCtx.internal);

        const rl::Rectangle dstRect = rl::Rectangle{
            (tile->coord.x * PIXELS_PER_TILE + layerPositionX) * VIRTUAL_SCREEN_RATIO,
            -(-tile->coord.y * PIXELS_PER_TILE + layerPositionY) * VIRTUAL_SCREEN_RATIO,
            tileSizeX,
            tileSizeY,
        };

        const TileRenderInfo& renderInfo = *tile->pTileRenderInfo;
        DrawTileHDR(invTexSizeX, invTexSizeY,
                    rl::Vector2{
                        renderInfo.sprite.atlasPosition.x,
                        renderInfo.sprite.atlasPosition.y,
                    },
                    dstRect, origin, renderInfo.orient.first + eCtx.transform->rotation, renderInfo.sprite.color.asRL(),
                    GetTileMetaFlags(renderInfo.sprite, eCtx.colorBuf.depth, eCtx.colorBuf.isUI, invTexSizeX, invTexSizeY), rl::rlGetCurrentDepth(),
                    renderInfo.orient.second == Facing::Left);
        rl::rlSetTexture(0);

    } else {
        // drawing all the tiles

        const TileMapLayer& layer = eCtx.entity.get<TileMapLayer>();
        if (layer.overlayTex.size() > 0) {
            // note: overlays not supported for y sorted layers
            // note: resetting shader on every draw call because the texture may have changed
            rl::BeginShaderMode(eCtx.shader->get());
            const rl::Texture& overlay = TextureManager::getTexture(layer.overlayTex);
            eCtx.shader->setTexture("_Overlay", overlay);
            eCtx.shader->setVector2(
                "_Scale", (Vector2f(1.0f / VIRTUAL_SCREEN_RATIO, 1.0f / VIRTUAL_SCREEN_RATIO) / Vector2f(overlay.width, overlay.height)).asRL());
            Graphics.setUniforms(eCtx.shader->get());
        }

        // Calculate which tiles are visible to the camera
        const Vector2i viewHalfTiles = Vector2i(WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME) / PIXELS_PER_TILE / 2 + 1;
        // const Vector2i viewHalfTiles = ctx.cameraViewHalf / PIXELS_PER_TILE;

        // caching these values once. Thousands of calls to shared_ptr_access really add up
        const s32 widthTiles = layer.tilemap->widthTiles;
        const s32 heightTiles = layer.tilemap->heightTiles;
        const stl::Map<u32, TileRenderInfo>& spriteCache = layer.tilemap->spriteCache;

        // these can be outside of the range ((0, widthTiles), (0, heightTiles))
        const s32 cameraTileX = static_cast<s32>(ctx.cameraPosition.x - layerPositionX) / PIXELS_PER_TILE;
        const s32 cameraTileY = static_cast<s32>(layerPositionY - ctx.cameraPosition.y) / PIXELS_PER_TILE;
        const s32 minX = std::max(0, cameraTileX - viewHalfTiles.x);
        const s32 maxX = std::min(widthTiles, cameraTileX + viewHalfTiles.x + 1);
        const s32 minY = std::max(0, cameraTileY - viewHalfTiles.y);
        const s32 maxY = std::min(heightTiles, cameraTileY + viewHalfTiles.y + 1);

        // group identical tiles so we can cache the complicated stuff
        rl::rlSetTexture(ctx.atlas.getTexture().id);
        u32 lastMask = -1;
        const TileRenderInfo* renderInfo;
        rl::Vector2 src;
        rl::Vector3 metaFlags;
        std::vector<TileInstance>& tiles = S_TILES_NORMAL[eCtx.entity];
        const u64 nTiles = tiles.size();
        const f32 z = rl::rlGetCurrentDepth();
        bool skipUntilNext = false;
        for (u64 i = 0; i < nTiles; ++i) {
            TileInstance tile = tiles[i];
            if (tile.x < minX || tile.x >= maxX || tile.y < minY || tile.y >= maxY) {
                continue;
            }
            if (tile.tileMask != lastMask) {
                lastMask = tile.tileMask;
                skipUntilNext = false;

                // using a dense map here (a vector of pairs) because it's much faster than std::unordered_map
                renderInfo = &spriteCache.get(tile.tileMask);
                if (ctx.isOccludersOnly && !renderInfo->isOccluder) {
                    skipUntilNext = true;
                    continue;
                }
                src = rl::Vector2{
                    renderInfo->sprite.atlasPosition.x,
                    renderInfo->sprite.atlasPosition.y,
                };
                metaFlags = GetTileMetaFlags(renderInfo->sprite, eCtx.colorBuf.depth, eCtx.colorBuf.isUI, invTexSizeX, invTexSizeY);
                // i could cache some stuff from DrawTileHDR here and inline the function in this loop, but profiling only showed a 2% speedup which
                // isn't worth the mess
            } else if (skipUntilNext) {
                continue;
            }

            rl::Rectangle dst = rl::Rectangle{
                (tile.x * PIXELS_PER_TILE + layerPositionX) * VIRTUAL_SCREEN_RATIO,
                -(-tile.y * PIXELS_PER_TILE + layerPositionY) * VIRTUAL_SCREEN_RATIO,
                tileSizeX,
                tileSizeY,
            };

            DrawTileHDR(invTexSizeX, invTexSizeY, src, dst, origin, renderInfo->orient.first + eCtx.transform->rotation,
                        renderInfo->sprite.color.asRL(), metaFlags, z, renderInfo->orient.second == Facing::Left);
        }

        rl::rlSetTexture(0);
        if (layer.overlayTex.size() > 0) {
            // note: resetting shader on every draw call because the texture may have changed
            rl::EndShaderMode();
        }
    }

    rl::rlEnd();
}

void TileRenderSystem::addToQueue(gfx::RenderQueue& queue) const {
    for (const auto& [entityid, entity] : getEntities()) {
        const auto& trans = entity.get<Transform>();
        const auto& tml = entity.get<TileMapLayer>();

        if (tml.isYSorted) {
            buildYsortList(entity, tml);
            std::vector<YsortedTileInfo>& sortedTiles = S_TILES_YSORTED[entity];
            const u64 nTiles = sortedTiles.size();
            // RESEARCH could be faster if I do what non-ysorted layers do during draw & get the tile bounds that are on screen and pass those to the
            // renderqueue directly. Would save hundreds of AABB lookups. Would need to make sure addPrecalculated adds to the occluder queue too.
            for (u64 i = 0; i < nTiles; ++i) {
                // override cached transform
                YsortedTileInfo& tile = sortedTiles[i];
                tile.renderInfo.transform = &trans;
                tile.renderInfo.internal = &tile;
                queue.add(tile.renderInfo);
            }
        } else {
            const Vector2i half = tml.sizeTiles * Vector2i(PIXELS_PER_TILE / 2, PIXELS_PER_TILE / 2);
            const Vector2i center = trans.positionPx + half * Vector2i(1, -1) + Vector2i(0, trans.floatHeight * FLOAT_HEIGHT_MULT);
            const auto bb = AABB(center, half);
            // occlusion calced at draw time. Pass MaybeInChildren so RenderQueue knows.
            queue.add(gfx::EntityPreRenderInfo{
                .boundingBox = bb,
                .transform = &trans,
                .ysortPosition = bb.bottom() - static_cast<s32>(trans.floatHeight * FLOAT_HEIGHT_MULT),
                .entity = entity,
                .isOccluder = gfx::EntityPreRenderInfo::IsOccluder::MaybeInChildren,
                .shader = tml.overlayTex.size() > 0 ? &ShaderMgr::get("TileOverlay") : nullptr,
            });
        }
    }
}

void TileRenderSystem::onAdd(ecs::Entity e) {
    // make sure all the tile sprites for this layer are in the cache
    const TileMapLayer& layer = e.get<TileMapLayer>();
    // caching these values once. Thousands of calls to shared_ptr_access really add up
    const s32 widthTiles = layer.tilemap->widthTiles;
    const s32 heightTiles = layer.tilemap->heightTiles;
    mDrawQueue.clear();
    for (s32 x = 0; x < widthTiles; x++) {
        for (s32 y = 0; y < heightTiles; y++) {
            const s32 ix = widthTiles * y + x;
            const u32 tileMask = layer.ids[ix];
            const TileInfo tile = getTile(tileMask);

            if (tile.gid == 0) {
                continue;  // empty tile
            }

            mDrawQueue.emplace_back(tileMask, x, y);

            if (!layer.tilemap->spriteCache.contains(tileMask)) {
                const auto sprite = getTileSprite(*layer.tilemap.get(), tile.gid).value();
                const auto orient = getOrientation(tile);
                layer.tilemap->spriteCache.insert({tileMask, TileRenderInfo{
                                                                 .sprite = sprite,
                                                                 .orient = orient,
                                                                 .isOccluder = layer.occlusionMask[ix],
                                                             }});
            }
        }
    }

    // have to cache the sort because it's very slow
    std::sort(mDrawQueue.begin(), mDrawQueue.end(),
              [](const TileInstance& tile1, const TileInstance& tile2) -> bool { return tile1.tileMask < tile2.tileMask; });
    S_TILES_NORMAL[e] = mDrawQueue;
}

void TileRenderSystem::onRemove(ecs::Entity e) {
    // erase from cache
    S_TILES_YSORTED.erase(e);
    S_TILES_NORMAL.erase(e);
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
    if (S_TILES_YSORTED.contains(e)) {
        return;
    }

    S_TILES_YSORTED[e] = {};
    const Transform& parentTrans = e.get<Transform>();
    const Vector2i halflen(PIXELS_PER_TILE / 2, PIXELS_PER_TILE / 2);
    const stl::Map<u32, TileRenderInfo>& spriteCache = tml.tilemap->spriteCache;

    // caching these values once. Thousands of calls to shared_ptr_access really add up
    const s32 widthTiles = tml.tilemap->widthTiles;
    const s32 heightTiles = tml.tilemap->heightTiles;
    for (s32 x = 0; x < widthTiles; x++) {
        for (s32 y = 0; y < heightTiles; y++) {
            const s32 ix = widthTiles * y + x;
            const TileInfo tile = getTile(tml.ids[ix]);

            if (tile.gid == 0) {
                continue;  // empty tile
            }

            const TileRenderInfo& tileRenderInfo = spriteCache.get(tml.ids[ix]);
            Vector2f worldPosition =
                Vector2f(x * PIXELS_PER_TILE, -y * PIXELS_PER_TILE) + parentTrans.position + Vector2f(0, parentTrans.floatHeight * FLOAT_HEIGHT_MULT);
            const AABB bb = AABB(worldPosition.round(), halflen);

            const auto ri = gfx::EntityPreRenderInfo{
                .boundingBox = bb,
                .transform = &parentTrans,
                .ysortPosition = bb.bottom() - static_cast<s32>(parentTrans.floatHeight * FLOAT_HEIGHT_MULT),
                .entity = e,
                .isOccluder = tileRenderInfo.isOccluder ? gfx::EntityPreRenderInfo::IsOccluder::Yes : gfx::EntityPreRenderInfo::IsOccluder::No,
                .shader = tml.overlayTex.size() > 0 ? &ShaderMgr::get("TileOverlay") : nullptr,
            };
            S_TILES_YSORTED[e].emplace_back(ri, &tileRenderInfo, Vector2i(x, y));
        }
    }
}

}  // namespace whal
