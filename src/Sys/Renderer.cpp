#include "Renderer.h"

#include <algorithm>
#include <raylib.h>
#include "raylib/src/rlgl.h"
#include "whalECS/src/ECS.h"

#include "Gfx/BaseShader.h"
#include "Gfx/RaylibUtil.h"
#include "Gfx/Texture.h"

#include "Components/Camera.h"
#include "Components/Tags.h"
#include "Components/Transform.h"

#include "Settings.h"
#include "Sys/System.h"

#include "Systems/ColliderSystem.h"
#include "Systems/Graphics/Common.h"
#include "Systems/LightSystem.h"

#include "Util/CameraUtil.h"
#include "Util/Color.h"
#include "Util/Print.h"

#include "Gfx/Shaders/LightDenoise.h"

namespace whal {

namespace gfx {

void applyShaders(rl::RenderTexture target, std::vector<std::shared_ptr<IShader>>& shaders) {
    rl::RenderTexture swap = Graphics.getTemporaryRT(target.texture);
    bool isSwapTarget = true;
    for (std::shared_ptr<IShader>& pShader : shaders) {
        if (isSwapTarget) {
            pShader->process(target, swap);
        } else {
            pShader->process(swap, target);
        }
        isSwapTarget = !isSwapTarget;
    }

    if (!isSwapTarget) {
        // make sure final image is on the target
        Graphics.blit(swap, target);
    }
    Graphics.releaseTemporaryRT(swap);
}

}  // namespace gfx

Renderer::Renderer() {
    mRaylibCamera.target = rl::Vector2(0.0f, 0.0f);
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

rl::RenderTexture Renderer::getTemporaryRT(s32 width, s32 height, rl::PixelFormat format, rl::TextureFilter filter) {
    // iterate backwards, since the most recently used stuff is in the back
    for (s32 i = static_cast<s32>(mAvailableRTs.size()) - 1; i >= 0; --i) {
        auto& it = mAvailableRTs[i];
        if (it.rt.texture.width == width && it.rt.texture.height == height && it.rt.texture.format == format) {
            // move this to mUsedRTs and return it
            auto result = mAvailableRTs[i];
            mAvailableRTs.erase(mAvailableRTs.begin() + i);
            if (result.filter != filter) {
                result.filter = filter;
                rl::SetTextureFilter(result.rt.texture, filter);
            }
            mUsedRTs.push_back(result);
            return result.rt;
        }
    }

    // Nothing was found, create a new RenderTexture
    rl::RenderTexture rt = rl::LoadRenderTextureFormat(width, height, format);
    rl::SetTextureFilter(rt.texture, filter);
    mUsedRTs.push_back({.rt = rt, .filter = filter});
    return rt;
}

rl::RenderTexture Renderer::getTemporaryRT(rl::Texture reference, rl::TextureFilter filter) {
    return getTemporaryRT(reference.width, reference.height, static_cast<rl::PixelFormat>(reference.format), filter);
}

void Renderer::releaseTemporaryRT(rl::RenderTexture rt) {
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
        print("Renderer::releaseTemporaryRT: Tried to release an already-released RenderTexture");
        return;
    }

    auto released = mUsedRTs[ix];
    mUsedRTs.erase(mUsedRTs.begin() + ix);
    released.unusedFrames = 0;
    mAvailableRTs.push_back(released);
}

void Renderer::blit(rl::RenderTexture src, rl::RenderTexture dst, rl::Shader shader) const {
    const rl::Rectangle srcRect = rl::Rectangle(0, 0, src.texture.width, -src.texture.height);
    const rl::Rectangle dstRect = rl::Rectangle(0, 0, dst.texture.width, dst.texture.height);

    rl::BeginTextureMode(dst);
    rl::ClearBackground(Colors::CLEAR);
    const bool isCustomShader = shader.id != 0;
    if (isCustomShader) {
        rl::BeginShaderMode(shader);
        rl::DrawTexturePro(src.texture, srcRect, dstRect, rl::Vector2{0, 0}, 0.0f, rl::WHITE);
        rl::EndShaderMode();
    } else {
        rl::DrawTexturePro(src.texture, srcRect, dstRect, rl::Vector2{0, 0}, 0.0f, rl::WHITE);
    }
    rl::EndTextureMode();
}

void Renderer::blit(rl::RenderTexture src, rl::RenderTexture dst, std::shared_ptr<IShader>& shader) {
    if (src.texture.width != dst.texture.width || src.texture.height != dst.texture.height) {
        // scale first, then apply shader
        rl::RenderTexture tmpSrc = getTemporaryRT(src.texture);
        blit(src, tmpSrc);

        shader->process(tmpSrc, dst);
        releaseTemporaryRT(tmpSrc);
    } else {
        shader->process(src, dst);
    }
}

void Renderer::blit(rl::RenderTexture src, rl::RenderTexture dst, IShader& shader) {
    if (src.texture.width != dst.texture.width || src.texture.height != dst.texture.height) {
        // scale first, then apply shader
        rl::RenderTexture tmpSrc = getTemporaryRT(src.texture);
        blit(src, tmpSrc);

        shader.process(tmpSrc, dst);
        releaseTemporaryRT(tmpSrc);
    } else {
        shader.process(src, dst);
    }
}

void Renderer::render() {
    // 0. Create render context and build the render queue.
    rl::Camera2D worldCamera = mRaylibCamera;
    ecs::Entity cameraEntity = *getCamera();
    worldCamera.rotation = cameraEntity.get<Transform2D>().rotationDegrees;
    const gfx::RenderContext renderContext{.cameraPosition = cameraEntity.get<PrecisePosition>().position,
                                           .camera = worldCamera,
                                           .atlas = TextureManager::getAtlas(TEXNAME_SPRITE),
                                           .cameraEntity = cameraEntity};
    buildRenderQueue(renderContext.cameraPosition.round());

    // 1. IRender and IRenderLight systems are drawn
    drawEntities(renderContext);  // drawn to TextureID::Staging
    drawLights(renderContext);    // drawn to TextureID::Lighting

    // 2. Renders everything to TextureID::Main
    rl::RenderTexture mainTex = TextureManager::getRenderTexture(TextureID::Main);
    rl::BeginTextureMode(mainTex);
    rl::ClearBackground(Colors::CLEAR);

    // Game Objects.
    gfx::DrawRenderTexture(mStagingTexture.tex);

    // Lights.
    rl::BeginBlendMode(rl::BLEND_MULTIPLIED);
    gfx::DrawRenderTexture(TextureManager::getRenderTexture(TextureID::Lighting));
    rl::EndBlendMode();

    // UI.
    drawUI(renderContext);
    rl::EndTextureMode();

    // 3. ? Apply post processing
    gfx::applyShaders(mainTex, cameraEntity.get<whal::Camera>().postEffects);
    // for (std::shared_ptr<BaseShader>& pShader : cameraEntity.get<whal::Camera>().postprocess) {
    //     pShader->process(mainTex, mainTex);
    // }

    // 4. Draw debug stuff.
#ifndef NDEBUG
    if (Input.isOn(InputType::DEBUG)) {
        rl::BeginTextureMode(mainTex);
        rl::BeginMode2D(worldCamera);
        // World.getSystem<DrawDebugSystem>()->drawEntities();
        drawColliders();
        rl::EndMode2D();
        rl::EndTextureMode();
    }
#endif
}

void Renderer::scaleDepthBuffers(gfx::RenderContext ctx) const {
    // Downscale the Multi-Render Target buffers to Game resolution (for lighting)
    const auto mt = Renderer::getStagingTex();
    const auto depthTex = mt.getDepth();
    const auto occlDepthTex = mt.getOcclusionDepth();
    const auto colorTex = mt.getOcclusionColor();

    const auto targetDepthTex = TextureManager::getRenderTexture(TextureID::AllDepth);
    const auto targetOcclDepthTex = TextureManager::getRenderTexture(TextureID::OcclusionDepth);
    const auto targetColorTex = TextureManager::getRenderTexture(TextureID::OcclusionColor);
    const rl::Rectangle srcRect = rl::Rectangle(0, 0, WINDOW_WIDTH_RENDER, -WINDOW_HEIGHT_RENDER);
    const rl::Rectangle dstRect = rl::Rectangle(0, 0, WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME);

    rl::BeginTextureMode(targetDepthTex);
    rl::ClearBackground(Colors::CLEAR);
    rl::DrawTexturePro(depthTex, srcRect, dstRect, rl::Vector2{0, 0}, 0.0f, rl::WHITE);
    rl::EndTextureMode();

    rl::BeginTextureMode(targetOcclDepthTex);
    rl::ClearBackground(Colors::CLEAR);
    rl::DrawTexturePro(occlDepthTex, srcRect, dstRect, rl::Vector2{0, 0}, 0.0f, rl::WHITE);
    rl::EndTextureMode();

    rl::BeginTextureMode(targetColorTex);
    rl::ClearBackground(Colors::CLEAR);
    rl::DrawTexturePro(colorTex, srcRect, dstRect, rl::Vector2{0, 0}, 0.0f, rl::WHITE);
    rl::EndTextureMode();
}

// this does what the old Mega-GraphicsSystem used to do.
void Renderer::drawEntities(gfx::RenderContext renderContext) {
    // Drawing GAME OBJECTS
    rl::BeginTextureMode(mStagingTexture.tex);
    rl::ClearBackground(Colors::CLEAR);
    rl::BeginMode2D(renderContext.camera);
    for (auto renderInfo : mRenderQueue) {
        renderInfo.piRender->draw(renderInfo, renderContext);
    }
    rl::EndMode2D();
    rl::EndTextureMode();

    scaleDepthBuffers(renderContext);
}

void Renderer::drawLights(gfx::RenderContext renderContext) {
    rl::RenderTexture lightTex =
        Graphics.getTemporaryRT(WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME, rl::PIXELFORMAT_UNCOMPRESSED_R16G16B16A16, rl::TEXTURE_FILTER_BILINEAR);
    rl::BeginTextureMode(lightTex);
    rl::BeginMode2D(renderContext.camera);
    rl::ClearBackground(rl::BLACK);

    rl::BeginBlendMode(rl::BLEND_ADDITIVE);
    for (const ecs::IRenderLight* pLightSystem : World.getLightSystems()) {
        pLightSystem->draw(renderContext);
    }
    // World.getSystem<PointLightSystem>()->drawEntities();
    // World.getSystem<BoxLightSystem>()->drawEntities();
    // World.getSystem<ShadowLightSystem>()->drawEntities();

    rl::EndBlendMode();
    rl::EndMode2D();
    rl::EndTextureMode();

    // TODO the camera should own this pipeline but idk how to design around the fact that lights are drawn at a lower resolution...
    static LightDenoise lightingPipeline;

    const auto lightTexUpscale = TextureManager::getRenderTexture(TextureID::Lighting);
    lightingPipeline.process(lightTex, lightTexUpscale);
    Graphics.releaseTemporaryRT(lightTex);
}

void Renderer::drawUI(const gfx::RenderContext ctx) const {
    rl::BeginMode2D(ctx.camera);
    for (auto renderInfo : mUIRenderQueue) {
        renderInfo.piRender->draw(renderInfo, ctx);
    }
    rl::EndMode2D();
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
