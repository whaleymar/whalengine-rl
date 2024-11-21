#include "Renderer.h"

#include <algorithm>
#include <raylib.h>
#include "Components/Camera.h"
#include "Gfx/BaseShader.h"
#include "Gfx/RaylibUtil.h"
#include "Systems/Graphics/Common.h"
#include "raylib/src/rlgl.h"
#include "whalECS/src/ECS.h"

#include "Components/Tags.h"
#include "Components/Transform.h"

#include "Gfx/Pipeline.h"
#include "Gfx/Texture.h"

#include "Settings.h"
#include "Sys/System.h"

#include "Systems/ColliderSystem.h"
#include "Systems/LightSystem.h"

#include "Util/CameraUtil.h"
#include "Util/Color.h"

namespace whal {

Renderer::Renderer() {
    mRaylibCamera.target = Vector2(0.0f, 0.0f);
    mRaylibCamera.zoom = 1.0f;
    mRaylibCamera.rotation = 0.0f;
}

void Renderer::init() {
    mStagingTexture = gfx::CreateMultiTexture();
}

void Renderer::tick() {
    const s32 RELEASE_FRAMES = 60;

    // iterate through the available RTs and get the range of ones that should be freed
    // RTs are ordered by unused frames (high to low) so as soon as we find an RT we should keep, we can exit
    s32 rmIx = -1;
    for (s32 i = 0; i < static_cast<s32>(mAvailableRTs.size()); ++i) {
        if (mAvailableRTs[i].unusedFrames >= RELEASE_FRAMES) {
            rmIx = i;
        } else {
            break;
        }
    }

    // release old RTs
    if (rmIx > -1) {
        // end is exclusive. add 1.
        mAvailableRTs.erase(mAvailableRTs.begin(), mAvailableRTs.begin() + rmIx + 1);
    }

    // increment unused frames
    for (size_t i = 0; i < mAvailableRTs.size(); ++i) {
        mAvailableRTs[i].unusedFrames++;
    }

    // Make used RTs available
    for (size_t i = 0; i < mUsedRTs.size(); ++i) {
        mUsedRTs[i].unusedFrames = 0;
        mAvailableRTs.push_back(mUsedRTs[i]);
    }

    // Clear used list
    mUsedRTs.clear();
}

RenderTexture Renderer::getTemporaryRT(s32 width, s32 height, PixelFormat format, TextureFilter filter) {
    // iterate backwards, since the most recently used stuff is in the back
    for (s32 i = static_cast<s32>(mAvailableRTs.size()) - 1; i >= 0; --i) {
        auto& it = mAvailableRTs[i];
        if (it.rt.texture.width == width && it.rt.texture.height == height && it.rt.texture.format == format) {
            // move this to mUsedRTs and return it
            auto result = mAvailableRTs[i];
            mAvailableRTs.erase(mAvailableRTs.begin() + i);
            if (result.filter != filter) {
                result.filter = filter;
                SetTextureFilter(result.rt.texture, filter);
            }
            mUsedRTs.push_back(result);
            return result.rt;
        }
    }

    // Nothing was found, create a new RenderTexture
    RenderTexture rt = LoadRenderTextureFormat(width, height, format);
    SetTextureFilter(rt.texture, filter);
    mUsedRTs.push_back({.rt = rt, .filter = filter});
    return rt;
}

RenderTexture Renderer::getTemporaryRT(Texture reference, TextureFilter filter) {
    return getTemporaryRT(reference.width, reference.height, static_cast<PixelFormat>(reference.format), filter);
}

void Renderer::releaseTemporaryRT(RenderTexture rt) {
    s32 ix = -1;
    // iterate in reverse since we are most likely to release a recently created one
    for (s32 i = static_cast<s32>(mUsedRTs.size()) - 1; i >= 0; --i) {
        if (mUsedRTs[i].rt.id == rt.id) {
            ix = i;
            break;
        }
    }

    if (ix == -1) {
        // user error, just exit
        return;
    }

    auto released = mUsedRTs[ix];
    mUsedRTs.erase(mUsedRTs.begin() + ix);
    released.unusedFrames = 0;
    mAvailableRTs.push_back(released);
}

void Renderer::render() {
    // 0. Create render context and build the render queue.
    Camera2D worldCamera = mRaylibCamera;
    ecs::Entity cameraEntity = *getCamera();
    worldCamera.rotation = cameraEntity.get<Transform2D>().rotationDegrees;
    const gfx::RenderContext renderContext{.cameraPosition = cameraEntity.get<PrecisePosition>().position,
                                           .camera = worldCamera,
                                           .atlas = TextureManager::getAtlas(TEXNAME_SPRITE),
                                           .cameraEntity = cameraEntity};
    buildRenderQueue(renderContext.cameraPosition.round());

    // 1. IRender and IRenderLight systems are drawn
    drawEntities(renderContext);  // drawn to TextureID::Staging
    drawLights(worldCamera);      // drawn to TextureID::Lighting

    // 2. Renders everything to TextureID::Main
    RenderTexture mainTex = TextureManager::getRenderTexture(TextureID::Main);
    BeginTextureMode(mainTex);
    ClearBackground(Colors::CLEAR);

    // Game Objects.
    gfx::DrawRenderTexture(mStagingTexture.tex);

    // Lights.
    BeginBlendMode(BLEND_MULTIPLIED);
    gfx::DrawRenderTexture(TextureManager::getRenderTexture(TextureID::Lighting));
    EndBlendMode();

    // UI.
    drawUI(renderContext);
    EndTextureMode();

    // 3. ? Apply post processing
    for (std::shared_ptr<BaseShader>& pShader : cameraEntity.get<whal::Camera>().postprocess) {
        pShader->process(mainTex, mainTex);
    }

    // 4. Draw debug stuff.
#ifndef NDEBUG
    if (Input.isOn(InputType::DEBUG)) {
        BeginTextureMode(mainTex);
        BeginMode2D(worldCamera);
        // World.getSystem<DrawDebugSystem>()->drawEntities();
        drawColliders();
        EndMode2D();
        EndTextureMode();
    }
#endif
}

void Renderer::scaleDepthBuffers(gfx::RenderContext ctx) const {
    // Downscale the Multi-Render Target buffers to Game resolution (for lighting)
    const auto mt = Renderer::getStagingTex();
    const Texture depthTex = Texture{
        .id = mt.depth,
        .width = WINDOW_WIDTH_RENDER,
        .height = WINDOW_HEIGHT_RENDER,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };
    const Texture occlDepthTex = Texture{
        .id = mt.occlusionDepth,
        .width = WINDOW_WIDTH_RENDER,
        .height = WINDOW_HEIGHT_RENDER,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };
    Texture colorTex = Texture{
        .id = mt.occlusionColor,
        .width = WINDOW_WIDTH_RENDER,
        .height = WINDOW_HEIGHT_RENDER,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };

    const auto targetDepthTex = TextureManager::getRenderTexture(TextureID::AllDepth);
    const auto targetOcclDepthTex = TextureManager::getRenderTexture(TextureID::OcclusionDepth);
    const auto targetColorTex = TextureManager::getRenderTexture(TextureID::OcclusionColor);
    const Rectangle srcRect = Rectangle(0, 0, WINDOW_WIDTH_RENDER, -WINDOW_HEIGHT_RENDER);
    const Rectangle dstRect = Rectangle(0, 0, WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME);

    BeginTextureMode(targetDepthTex);
    ClearBackground(Colors::CLEAR);
    DrawTexturePro(depthTex, srcRect, dstRect, Vector2{0, 0}, 0.0f, WHITE);
    EndTextureMode();

    BeginTextureMode(targetOcclDepthTex);
    ClearBackground(Colors::CLEAR);
    DrawTexturePro(occlDepthTex, srcRect, dstRect, Vector2{0, 0}, 0.0f, WHITE);
    EndTextureMode();

    BeginTextureMode(targetColorTex);
    ClearBackground(Colors::CLEAR);
    DrawTexturePro(colorTex, srcRect, dstRect, Vector2{0, 0}, 0.0f, WHITE);
    EndTextureMode();
}

// this does what the old Mega-GraphicsSystem used to do.
void Renderer::drawEntities(gfx::RenderContext renderContext) {
    // Drawing GAME OBJECTS
    BeginTextureMode(mStagingTexture.tex);
    ClearBackground(Colors::CLEAR);
    BeginMode2D(renderContext.camera);
    for (auto renderInfo : mRenderQueue) {
        renderInfo.piRender->draw(renderInfo, renderContext);
    }
    EndMode2D();
    EndTextureMode();

    scaleDepthBuffers(renderContext);
}

void Renderer::drawUI(const gfx::RenderContext ctx) const {
    BeginMode2D(ctx.camera);
    for (auto renderInfo : mUIRenderQueue) {
        renderInfo.piRender->draw(renderInfo, ctx);
    }
    EndMode2D();
}

static bool isBelow(const gfx::EntityRenderInfo& entity1, const gfx::EntityRenderInfo& entity2) {
    if (entity1.preciseTransform.depth != entity2.preciseTransform.depth) {
        return entity1.preciseTransform.depth < entity2.preciseTransform.depth;
    }

    if constexpr (WORLD_TYPE == WorldType2D::TopDown) {
        return entity1.boundingBox.bottom() > entity2.boundingBox.bottom();
    } else {
        return false;  // doesn't really matter
    }
}

void Renderer::buildRenderQueue(Vector2i cameraPosition) {
    // .clear() doesn't affect capacity
    mRenderQueue.clear();
    mUIRenderQueue.clear();
    static std::vector<gfx::EntityRenderInfo> tmpDrawList;  // make it static to minimize memory allocations per frame
    tmpDrawList.clear();

    const AABB cameraViewBox(cameraPosition, {WINDOW_WIDTH_GAME / 2 + PIXELS_PER_TILE, WINDOW_HEIGHT_GAME / 2 + PIXELS_PER_TILE});
    for (const ecs::RenderSystemPair& renderSystem : World.getRenderSystems()) {
        tmpDrawList.reserve(renderSystem.pSystem->getEntitiesVirtual().size());  // reserve space in case capacity is too low
        renderSystem.pIRender->addToQueue(tmpDrawList);
        for (const gfx::EntityRenderInfo& renderInfo : tmpDrawList) {
            // Filter out hidden entities and entities outside of the viewport
            if (!renderInfo.entity.has<Invisible>() && cameraViewBox.isOverlapping(renderInfo.boundingBox)) {
                if (renderInfo.preciseTransform.depth == Depth::Debug || renderInfo.preciseTransform.depth == Depth::UIFar ||
                    renderInfo.preciseTransform.depth == Depth::UIClose) {
                    mUIRenderQueue.emplace_back(renderInfo.boundingBox, renderInfo.preciseTransform, renderInfo.entity, renderInfo.piRender,
                                                gfx::ColorBufInfo{.depth = static_cast<u8>(renderInfo.preciseTransform.depth),
                                                                  .isOccluder = renderInfo.entity.has<BlocksLight>(),
                                                                  .isUI = true});
                } else {
                    mRenderQueue.emplace_back(renderInfo.boundingBox, renderInfo.preciseTransform, renderInfo.entity, renderInfo.piRender,
                                              gfx::ColorBufInfo{.depth = static_cast<u8>(renderInfo.preciseTransform.depth),
                                                                .isOccluder = renderInfo.entity.has<BlocksLight>(),
                                                                .isUI = false});
                }
            }
        }

        tmpDrawList.clear();
    }

    std::sort(mRenderQueue.begin(), mRenderQueue.end(), isBelow);
    std::sort(mUIRenderQueue.begin(), mUIRenderQueue.end(), isBelow);
}

}  // namespace whal
