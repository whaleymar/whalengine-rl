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
#include "Sys/System.h"
#include "Util/CameraUtil.h"
#include "raylib.h"
#include "rlgl.h"

namespace whal {

static std::pair<f32, Facing> getOrientation(TileInfo tile);
static void buildYsortList(ecs::Entity e, const TileMapLayer& tml);
static std::unordered_map<ecs::Entity, std::vector<Vector2i>, ecs::EntityHash> S_YSORT_COORD_LUT;
static std::unordered_map<ecs::Entity, std::vector<TileInstance>, ecs::EntityHash> S_TILE_BATCH;  // for non-ysorted layers
static std::unordered_map<ecs::Entity, std::vector<gfx::EntityPreRenderInfo>, ecs::EntityHash> S_YSORT_RENDERINFO_LUT;

// NOTE: assumptions made when drawing tiles:
/*

- tiles don't move independently of their TileMapLayer
- tile IDs can't be swapped once added to this system (can be circumvented if I add a cache invalidation mechanism, or by deactivating/reactivating)

*/

// slightly optimized version of DrawSpriteHDR
static void DrawTileHDR(float invTexWidth, float invTexHeight, rl::Vector2 source, rl::Rectangle dest, rl::Vector2 origin, float rotation,
                        rl::Vector4 hdrColor, rl::Vector3 packedCBI, bool flipX) {
    rl::Vector2 topLeft;
    rl::Vector2 topRight;
    rl::Vector2 bottomLeft;
    rl::Vector2 bottomRight;

    // Only calculate rotation if needed
    if (rotation == 0.0f) {
        float x = dest.x - origin.x;
        float y = dest.y - origin.y;
        topLeft = (rl::Vector2){x, y};
        topRight = (rl::Vector2){x + dest.width, y};
        bottomLeft = (rl::Vector2){x, y + dest.height};
        bottomRight = (rl::Vector2){x + dest.width, y + dest.height};
    } else {
        float sinRotation = sinf(rotation * DEG2RAD);
        float cosRotation = cosf(rotation * DEG2RAD);
        float x = dest.x;
        float y = dest.y;
        float dx = -origin.x;
        float dy = -origin.y;

        topLeft.x = x + dx * cosRotation - dy * sinRotation;
        topLeft.y = y + dx * sinRotation + dy * cosRotation;

        topRight.x = x + (dx + dest.width) * cosRotation - dy * sinRotation;
        topRight.y = y + (dx + dest.width) * sinRotation + dy * cosRotation;

        bottomLeft.x = x + dx * cosRotation - (dy + dest.height) * sinRotation;
        bottomLeft.y = y + dx * sinRotation + (dy + dest.height) * cosRotation;

        bottomRight.x = x + (dx + dest.width) * cosRotation - (dy + dest.height) * sinRotation;
        bottomRight.y = y + (dx + dest.width) * sinRotation + (dy + dest.height) * cosRotation;
    }

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
    ecs::Entity layerEntity = eCtx.entity;
    const TileMapLayer& layer = layerEntity.get<TileMapLayer>();
    const Vector2f tileSize = (Vector2f(PIXELS_PER_TILE, PIXELS_PER_TILE) * VIRTUAL_SCREEN_RATIO * eCtx.transform.scale).absolute();
    const rl::Vector2 origin = (tileSize * Vector2f(0.5, 0.5)).asRL();

    // caching these values once. Thousands of calls to shared_ptr_access really add up
    const s32 widthTiles = layer.tilemap->widthTiles;
    const s32 heightTiles = layer.tilemap->heightTiles;
    const stl::Map<s32, TileRenderInfo>& spriteCache = layer.tilemap->spriteCache;

    // this shader could be slightly faster & more ergonomic if I make it a Shader class
    if (layer.overlayTex.size() > 0) {
        // note: overlays will be slow for Y sorted layers
        auto overlayLoc = rl::GetShaderLocation(eCtx.shader, "_Overlay");
        const rl::Texture& overlay = TextureManager::getTexture(layer.overlayTex);
        rl::SetShaderValueTexture(eCtx.shader, overlayLoc, overlay);
        auto scaleLoc = rl::GetShaderLocation(eCtx.shader, "_Scale");
        rl::Vector2 scale = (Vector2f(1.0f / VIRTUAL_SCREEN_RATIO, 1.0f / VIRTUAL_SCREEN_RATIO) / Vector2f(overlay.width, overlay.height)).asRL();
        rl::SetShaderValue(eCtx.shader, scaleLoc, &scale, rl::SHADER_UNIFORM_VEC2);
        auto timeLoc = rl::GetShaderLocation(eCtx.shader, "_Time");
        f32 time = Time.getElapsed();
        rl::SetShaderValue(eCtx.shader, timeLoc, &time, rl::SHADER_UNIFORM_FLOAT);
    }

    const Vector2f invTexDims(1.0f / static_cast<f32>(ctx.atlas.getTexture().width), 1.0f / static_cast<f32>(ctx.atlas.getTexture().height));

    if (layer.isYSorted) {
        // just drawing one tile
        const s32 ySortIx = eCtx.internal;
        const Vector2i coord = S_YSORT_COORD_LUT[eCtx.entity][ySortIx];
        const s32 ix = widthTiles * coord.y + coord.x;
        rl::rlSetTexture(ctx.atlas.getTexture().id);
        auto tile = getTile(layer.ids[ix]);

        const TileRenderInfo& renderInfo = spriteCache.get(tile.gid);
        const auto src = rl::Vector2{
            renderInfo.sprite.atlasPosition.x,
            renderInfo.sprite.atlasPosition.y,
        };

        const Vector2f worldPosition = Vector2f(coord.x * PIXELS_PER_TILE, -coord.y * PIXELS_PER_TILE) + eCtx.transform.position;

        const rl::Rectangle dstRect = rl::Rectangle{
            worldPosition.x * VIRTUAL_SCREEN_RATIO,
            -worldPosition.y * VIRTUAL_SCREEN_RATIO,
            tileSize.x,
            tileSize.y,
        };

        DrawTileHDR(invTexDims.x, invTexDims.y, src, dstRect, origin, renderInfo.orient.first + eCtx.transform.rotation,
                    renderInfo.sprite.color.asRL(),
                    GetTileMetaFlags(renderInfo.sprite, eCtx.colorBuf.depth, eCtx.colorBuf.isUI, invTexDims.x, invTexDims.y),
                    renderInfo.orient.second == Facing::Left);
        rl::rlSetTexture(0);

    } else {
        // drawing all the tiles

        // Calculate which tiles are visible to the camera
        const s32 viewWidthHalfTiles = WINDOW_WIDTH_GAME / PIXELS_PER_TILE / 2 + 1;
        const s32 viewHeightHalfTiles = WINDOW_HEIGHT_GAME / PIXELS_PER_TILE / 2 + 1;

        // these can be outside of the range ((0, widthTiles), (0, heightTiles))
        const s32 cameraTileX = static_cast<s32>(ctx.cameraPosition.x - eCtx.transform.position.x) / PIXELS_PER_TILE;
        const s32 cameraTileY = static_cast<s32>(eCtx.transform.position.y - ctx.cameraPosition.y) / PIXELS_PER_TILE;
        const s32 minX = std::max(0, cameraTileX - viewWidthHalfTiles);
        const s32 maxX = std::min(widthTiles, cameraTileX + viewWidthHalfTiles + 1);
        const s32 minY = std::max(0, cameraTileY - viewHeightHalfTiles);
        const s32 maxY = std::min(heightTiles, cameraTileY + viewHeightHalfTiles + 1);

        // group identical tiles so we can cache the complicated stuff
        rl::rlSetTexture(ctx.atlas.getTexture().id);
        u32 lastMask = -1;
        TileInfo tInfo;
        const TileRenderInfo* renderInfo;
        rl::Vector2 src;
        rl::Vector3 metaFlags;
        std::vector<TileInstance>& tiles = S_TILE_BATCH[layerEntity];
        const u64 nTiles = tiles.size();
        bool skipUntilNext = false;
        for (u64 i = 0; i < nTiles; ++i) {
            TileInstance tile = tiles[i];
            if (tile.x < minX || tile.x >= maxX || tile.y < minY || tile.y >= maxY) {
                continue;
            }
            if (tile.tileMask != lastMask) {
                lastMask = tile.tileMask;
                skipUntilNext = false;
                tInfo = getTile(tile.tileMask);

                // using a dense map here (a vector of pairs) because it's much faster than std::unordered_map
                renderInfo = &spriteCache.get(tInfo.gid);
                if (ctx.isOccludersOnly && !renderInfo->isOccluder) {
                    skipUntilNext = true;
                    continue;
                }
                src = rl::Vector2{
                    renderInfo->sprite.atlasPosition.x,
                    renderInfo->sprite.atlasPosition.y,
                };
                metaFlags = GetTileMetaFlags(renderInfo->sprite, eCtx.colorBuf.depth, eCtx.colorBuf.isUI, invTexDims.x, invTexDims.y);
                // i could cache some stuff from DrawTileHDR here and inline the function in this loop, but profiling only showed a 2% speedup which
                // isn't worth the mess
            } else if (skipUntilNext) {
                continue;
            }

            const Vector2f worldPosition = Vector2f(tile.x * PIXELS_PER_TILE, -tile.y * PIXELS_PER_TILE) + eCtx.transform.position;
            rl::Rectangle dst = rl::Rectangle{
                worldPosition.x * VIRTUAL_SCREEN_RATIO,
                -worldPosition.y * VIRTUAL_SCREEN_RATIO,
                tileSize.x,
                tileSize.y,
            };

            DrawTileHDR(invTexDims.x, invTexDims.y, src, dst, origin, renderInfo->orient.first + eCtx.transform.rotation,
                        renderInfo->sprite.color.asRL(), metaFlags, renderInfo->orient.second == Facing::Left);
        }

        rl::rlSetTexture(0);
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
            const auto& sortedTiles = S_YSORT_RENDERINFO_LUT[entity];
            const u64 nTiles = sortedTiles.size();
            for (u64 i = 0; i < nTiles; ++i) {
                queue.add(sortedTiles[i]);
            }
        } else {
            // occlusion calced at draw time. Pass MaybeInChildren so RenderQueue knows.
            queue.add(gfx::EntityPreRenderInfo{
                .boundingBox = bb,
                .transform = trans,
                .entity = entity,
                .isOccluder = gfx::EntityPreRenderInfo::IsOccluder::MaybeInChildren,
                .shader = tml.overlayTex.size() > 0 ? ShaderManager::get(Shaders::Overlay) : rl::Shader{.id = 0xffffffff, .locs = nullptr},
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

    // have to cache the sort because it's very slow
    std::sort(mDrawQueue.begin(), mDrawQueue.end(),
              [](const TileInstance& tile1, const TileInstance& tile2) -> bool { return tile1.tileMask < tile2.tileMask; });
    S_TILE_BATCH[e] = mDrawQueue;
}

void TileRenderSystem::onRemove(ecs::Entity e) {
    // erase from cache
    S_YSORT_RENDERINFO_LUT.erase(e);
    S_YSORT_COORD_LUT.erase(e);
    S_TILE_BATCH.erase(e);
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
    const stl::Map<s32, TileRenderInfo>& spriteCache = tml.tilemap->spriteCache;

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

            S_YSORT_COORD_LUT[e].push_back(Vector2i(x, y));
            Vector2f worldPosition = Vector2f(x * PIXELS_PER_TILE, -y * PIXELS_PER_TILE) + parentTrans.position;

            const auto ri = gfx::EntityPreRenderInfo{
                .boundingBox = AABB(worldPosition.round(), halflen),
                .transform = parentTrans,
                .entity = e,
                .isOccluder =
                    spriteCache.get(tile.gid).isOccluder ? gfx::EntityPreRenderInfo::IsOccluder::Yes : gfx::EntityPreRenderInfo::IsOccluder::No,
                .internal = lut_ix,
                .shader = tml.overlayTex.size() > 0 ? ShaderManager::get(Shaders::Overlay) : rl::Shader{.id = 0xffffffff, .locs = nullptr},
            };
            S_YSORT_RENDERINFO_LUT[e].push_back(ri);

            lut_ix++;
        }
    }
}

}  // namespace whal
