#include "Renderer.h"

#include <algorithm>
#include <raylib.h>
#include "Components/Camera.h"
#include "Gfx/RaylibUtil.h"
#include "Systems/Graphics/Common.h"
#include "Util/Print.h"
#include "raylib/src/rlgl.h"
#include "whalECS/src/ECS.h"

#include "Components/Tags.h"
#include "Components/Transform.h"

#include "Gfx/Pipeline.h"
#include "Gfx/ShaderManager.h"
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
    mStagingTexture = gfx::CreateMultiTexture();
}

void Renderer::init() {
    instance()._init();
}

void Renderer::render() {
    instance()._render();
}

void Renderer::setPostEffects(Pipeline pipeline) {
    instance().mPostProcessSteps = std::move(pipeline);
}

void Renderer::onEvent(evt::ShaderReload) {
    mMainTextureUniform = GetShaderLocation(ShaderManager::get(Shaders::PostProcess), "iMainTex");
    mBloomThresholdUniform = GetShaderLocation(ShaderManager::get(Shaders::Threshold), "lum_threshold");
    // mExposureUniform = GetShaderLocation(ShaderManager::get(Shaders::ToneMap), "exposure");
}

// Just logs that the renderer started. This is here so the singleton registers its listeners before other stuff starts happening
void Renderer::_init() const {
    print("Initialized Renderer");
}

void Renderer::_render() {
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
    TextureManager::instance().renderBackgroundTextures();              // drawn to TextureID::Background
    _drawEntities(renderContext);                                       // drawn to TextureID::Staging
    drawLights(worldCamera);                                            // drawn to TextureID::UpscaledLighting
    World.getSystem<RadianceLightSystem>()->drawEntities(worldCamera);  // drawn to TextureID::Radiance

    // 2. Renders everything to TextureID::Main
    BeginTextureMode(TextureManager::getRenderTexture(TextureID::Main));
    ClearBackground(Colors::CLEAR);

    // Game Objects.
    gfx::DrawRenderTexture(mStagingTexture.tex);

    // Lights.
    BeginBlendMode(BLEND_MULTIPLIED);
    gfx::DrawRenderTexture(TextureManager::getRenderTexture(TextureID::UpscaledLighting));
    EndBlendMode();

    // Radiance. Is not upscaled.
    RenderTexture2D radianceTexture = TextureManager::getRenderTexture(TextureID::Radiance);
    Rectangle screenSourceRec =
        Rectangle(0.0f, 0.0f, static_cast<f32>(radianceTexture.texture.width), -static_cast<f32>(radianceTexture.texture.height));
    Rectangle dstRect(0, 0, radianceTexture.texture.width * VIRTUAL_SCREEN_RATIO, radianceTexture.texture.height * VIRTUAL_SCREEN_RATIO);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawTexturePro(radianceTexture.texture, screenSourceRec, dstRect, {0.0f, 0.0f}, 0.0f, WHITE);
    EndBlendMode();

    _drawUI(renderContext);
    EndTextureMode();

    // 3. ? Apply post processing
    _bloomAndTonemap(renderContext);
    mPostProcessSteps.process(TextureID::Main);

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

static void scaleTexture(TextureID src, TextureID dst, BlendMode blendMode = BLEND_ALPHA /*, TextureFilter filter*/) {
    const auto srcTex = TextureManager::getRenderTexture(src);
    const auto dstTex = TextureManager::getRenderTexture(dst);
    const Rectangle srcRect = Rectangle(0, 0, srcTex.texture.width, -srcTex.texture.height);
    const Rectangle dstRect = Rectangle(0, 0, dstTex.texture.width, dstTex.texture.height);

    BeginTextureMode(dstTex);
    BeginBlendMode(blendMode);
    ClearBackground(Colors::CLEAR);
    DrawTexturePro(srcTex.texture, srcRect, dstRect, Vector2{0, 0}, 0.0f, WHITE);
    EndBlendMode();
    EndTextureMode();
}

void Renderer::_drawOcclusionMask(gfx::RenderContext ctx) const {
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
void Renderer::_drawEntities(gfx::RenderContext renderContext) {
    _drawOcclusionMask(renderContext);

    // Drawing GAME OBJECTS
    BeginTextureMode(mStagingTexture.tex);
    ClearBackground(Colors::CLEAR);
    BeginMode2D(renderContext.camera);
    for (auto renderInfo : mRenderQueue) {
        renderInfo.piRender->draw(renderInfo, renderContext);
    }
    EndMode2D();
    EndTextureMode();
}

void Renderer::_drawUI(const gfx::RenderContext ctx) const {
    BeginMode2D(ctx.camera);
    for (auto renderInfo : mUIRenderQueue) {
        renderInfo.piRender->draw(renderInfo, ctx);
    }
    EndMode2D();
}

static bool isBelow(const gfx::EntityRenderInfo& entity1, const gfx::EntityRenderInfo& entity2) {
    if (entity1.preciseTransform.depth != entity2.preciseTransform.depth) {
        return depthToFloat(entity1.preciseTransform.depth) < depthToFloat(entity2.preciseTransform.depth);
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
    // int nCulled = 0;
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
                // } else {
                //     nCulled++;
            }
        }

        tmpDrawList.clear();
    }
    // print("Culled ", nCulled, "on frame", Time.getFrame());

    std::sort(mRenderQueue.begin(), mRenderQueue.end(), isBelow);
    std::sort(mUIRenderQueue.begin(), mUIRenderQueue.end(), isBelow);
}

// RESEARCH should be part of post processing pipeline.
// Separate right now because it uses FBOs w/ different resolutions

// RESEARCH the LearnOpenGL bloom tutorial uses (equiv of) `rlActiveDrawBuffers` to do the thresholding step *while* lighting.
// Could try this for a speed boost
void Renderer::_bloomAndTonemap(const gfx::RenderContext& ctx) const {
    // BLOOM
    auto bloomTexUS = TextureManager::getRenderTexture(TextureID::UpscaledBloom);
    auto mainTex = TextureManager::getRenderTexture(TextureID::Main);
    // auto tmpTex = mStagingTexture.tex;
    // This can be anything with the render dimensions EXCEPT mStagingTexture.tex, because I want to maintain the other color buffers for debugging
    auto tmpTex = TextureManager::getRenderTexture(TextureID::UpscaledLighting);

    // 1. Threshold the Main tex

    const f32 threshold = ctx.cameraEntity.get<whal::Camera>().bloomThreshold;
    BeginTextureMode(bloomTexUS);
    Shader threshShader = ShaderManager::get(Shaders::Threshold);
    BeginShaderMode(threshShader);
    SetShaderValue(threshShader, mBloomThresholdUniform, &threshold, SHADER_UNIFORM_FLOAT);
    gfx::DrawRenderTexture(mainTex);
    EndShaderMode();
    EndTextureMode();

    // 2. Downscale and upscale for a cheap blur
    scaleTexture(TextureID::UpscaledBloom, TextureID::DownscaledBloom);
    scaleTexture(TextureID::DownscaledBloom, TextureID::UpscaledBloom);

    // 3. Draw additively
    // RESEARCH could save a draw call by making the final staging -> main thingy happen here?
    BeginTextureMode(mainTex);
    BeginBlendMode(BLEND_ADDITIVE);
    gfx::DrawRenderTexture(bloomTexUS);
    EndBlendMode();
    EndTextureMode();

    // TONE MAPPING
    BeginTextureMode(tmpTex);
    ClearBackground(Colors::CLEAR);
    BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);  // doesn't seem to make a difference
    const auto shader = ShaderManager::get(Shaders::ToneMap);
    BeginShaderMode(shader);
    gfx::DrawRenderTexture(mainTex, WHITE);
    EndShaderMode();
    EndBlendMode();
    EndTextureMode();

    // write back to main
    BeginTextureMode(mainTex);
    ClearBackground(Colors::CLEAR);
    BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
    gfx::DrawRenderTexture(tmpTex);
    EndBlendMode();
    EndTextureMode();
}

}  // namespace whal
