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

    // 1. ECS systems with draw-like methods are updated (this should probably happen automatically)
    TextureManager::instance().renderBackgroundTextures();  // drawn to TextureID::Background
    drawEntities(renderContext);                            // drawn to TextureID::Staging
    drawLights(worldCamera);                                // drawn to TextureID::UpscaledLighting

    // 2. Renders everything to TextureID::Main
    RenderTexture mainTex = TextureManager::getRenderTexture(TextureID::Main);
    BeginTextureMode(mainTex);
    ClearBackground(Colors::CLEAR);

    // Game Objects.
    gfx::DrawRenderTexture(mStagingTexture.tex);

    // Lights.
    BeginBlendMode(BLEND_MULTIPLIED);
    gfx::DrawRenderTexture(TextureManager::getRenderTexture(TextureID::UpscaledLighting));
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
        BeginTextureMode(TextureManager::getRenderTexture(TextureID::Main));
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
