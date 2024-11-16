#include "Renderer.h"

#include <algorithm>
#include <raylib.h>
#include "Gfx/RaylibUtil.h"
#include "Util/Print.h"
#include "raylib/src/rlgl.h"
#include "whalECS/src/ECS.h"

#include "Components/GfxFlags.h"
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

static Color getPostProcessFlags(gfx::EntityRenderInfo renderInfo);

Renderer::Renderer() {
    mRaylibCamera.target = Vector2(0.0f, 0.0f);
    mRaylibCamera.zoom = 1.0f;
    mRaylibCamera.rotation = 0.0f;
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
    const gfx::RenderContext renderContext{
        .cameraPosition = cameraEntity.get<PrecisePosition>().position, .camera = worldCamera, .atlas = TextureManager::getAtlas(TEXNAME_SPRITE)};
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
    gfx::DrawRenderTexture(TextureManager::getRenderTexture(TextureID::Staging));

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

    // TONE MAPPING
    // TODO want to do bloom right before this and then additively blend & do tone mapping
    // static f32 s_exposure = 1.0;
    BeginTextureMode(TextureManager::getRenderTexture(TextureID::Staging));
    ClearBackground(Colors::CLEAR);
    BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);  // doesn't seem to make a difference
    const auto shader = ShaderManager::get(Shaders::ToneMap);
    BeginShaderMode(shader);
    // SetShaderValue(shader, mExposureUniform, &s_exposure, SHADER_UNIFORM_FLOAT);
    gfx::DrawRenderTexture(TextureManager::getRenderTexture(TextureID::Main), WHITE);
    EndShaderMode();
    EndBlendMode();
    EndTextureMode();

    // write back to main
    BeginTextureMode(TextureManager::getRenderTexture(TextureID::Main));
    ClearBackground(Colors::CLEAR);
    BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
    gfx::DrawRenderTexture(TextureManager::getRenderTexture(TextureID::Staging));
    EndBlendMode();
    EndTextureMode();

    // 3. ? Apply post processing
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

    // TODO not sure whether I should apply this to src or dst
    // TODO this has side effects, figure out how to undo the filter change afterwards
    // SetTextureFilter(fullResTex.texture, filter);

    BeginTextureMode(dstTex);
    BeginBlendMode(blendMode);
    ClearBackground(Colors::CLEAR);
    DrawTexturePro(srcTex.texture, srcRect, dstRect, Vector2{0, 0}, 0.0f, WHITE);
    EndBlendMode();
    EndTextureMode();
}

// This draws the effects mask to TextureID::DownscaledPostProcess.
// It also populates mOcclusionQueue
void Renderer::_drawEffectsMask(gfx::RenderContext renderContext) {
    // Draw to Effects Buffer (using main texture for this as it's unused at this point in the render pipeline)
    constexpr Color NO_EFFECT = Color{0, 0, 0, 0};
    mOcclusionQueue.clear();

    BeginTextureMode(TextureManager::getRenderTexture(TextureID::Main));
    ClearBackground(NO_EFFECT);
    BeginMode2D(renderContext.camera);
    ShaderManager::activate(Shaders::Silhouette);
    for (auto renderInfo : mRenderQueue) {
        if (renderInfo.piRender->isPostProcessingUsed()) {
            if (renderInfo.entity.has<BlocksLight>()) {
                mOcclusionQueue.push_back(renderInfo);
            }
            const Color flags = getPostProcessFlags(renderInfo);
            renderContext.colorOverride = flags;
            renderInfo.piRender->draw(renderInfo, renderContext);
        }
    }
    EndShaderMode();
    EndMode2D();
    EndTextureMode();
    renderContext.colorOverride = Corrade::Containers::NullOpt;

    // Draw downscaled version of the post-process texture
    // makes it much faster since the PP shader is SLOW.
    scaleTexture(TextureID::Main, TextureID::DownscaledPostProcess);
}

void Renderer::_drawOcclusionMask(gfx::RenderContext ctx) const {
    // DRAW COLOR INFO TO OCCLUSION TEXTURE
    ctx.colorOverride = Corrade::Containers::NullOpt;
    BeginTextureMode(TextureManager::getRenderTexture(TextureID::Main));
    BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
    ClearBackground(Colors::CLEAR);
    BeginMode2D(ctx.camera);
    for (auto renderInfo : mOcclusionQueue) {
        renderInfo.piRender->draw(renderInfo, ctx);
    }
    EndMode2D();
    EndBlendMode();
    EndTextureMode();

    // DOWNSCALE
    scaleTexture(TextureID::Main, TextureID::OcclusionColor, BLEND_ALPHA_PREMULTIPLY);

    // DRAW DEPTH INFO TO OTHER OCCLUSION TEXTURE
    // "Mom, can we use the `RenderTexture.depth`?"
    // "We have `RenderTexture.depth` at home."
    // `RenderTexture.depth` at home:
    BeginTextureMode(TextureManager::getRenderTexture(TextureID::Main));
    ClearBackground(Colors::CLEAR);
    BeginMode2D(ctx.camera);
    ShaderManager::activate(Shaders::Silhouette);
    for (auto renderInfo : mOcclusionQueue) {
        f32 d = depthToFloat(renderInfo.preciseTransform.depth);
        const Color color = ColorFromNormalized(Vector4{d, 0.0f, 0.0f, 1.0f});
        ctx.colorOverride = color;
        renderInfo.piRender->draw(renderInfo, ctx);
    }
    EndShaderMode();
    EndMode2D();
    EndTextureMode();

    // DOWNSCALE
    scaleTexture(TextureID::Main, TextureID::OcclusionDepth);

    // DRAW DEPTH INFO FOR EVERYTHING TO LAST TEXTURE
    BeginTextureMode(TextureManager::getRenderTexture(TextureID::Main));
    ClearBackground(BLACK);
    BeginMode2D(ctx.camera);
    ShaderManager::activate(Shaders::Silhouette);
    for (auto renderInfo : mRenderQueue) {
        if (!renderInfo.piRender->isPostProcessingUsed()) {
            continue;
        }
        f32 d = depthToFloat(renderInfo.preciseTransform.depth);
        const Color color = ColorFromNormalized(Vector4{d, 0.0f, 0.0f, 1.0f});
        ctx.colorOverride = color;
        renderInfo.piRender->draw(renderInfo, ctx);
    }
    EndShaderMode();
    EndMode2D();
    EndTextureMode();

    // DOWNSCALE
    scaleTexture(TextureID::Main, TextureID::AllDepth);
}

// this does what the old Mega-GraphicsSystem used to do.
void Renderer::_drawEntities(gfx::RenderContext renderContext) {
    _drawEffectsMask(renderContext);
    _drawOcclusionMask(renderContext);

    // Drawing GAME OBJECTS
    BeginTextureMode(TextureManager::getRenderTexture(TextureID::Staging));
    ClearBackground(Colors::CLEAR);
    BeginMode2D(renderContext.camera);
    for (auto renderInfo : mRenderQueue) {
        renderInfo.piRender->draw(renderInfo, renderContext);
    }
    EndMode2D();
    EndTextureMode();

    // Apply Post Processing Effects
    // Lighting and Radiance textures are unused at this point, so I use them as a temporary downscaled render target

    // 1. draw downscaled version of Staging
    scaleTexture(TextureID::Staging, TextureID::Radiance);
    // /1.

    // 2. Render effects to new buffer
    RenderTexture effectsTarget = TextureManager::getRenderTexture(TextureID::Lighting);
    BeginTextureMode(effectsTarget);
    ClearBackground(Colors::CLEAR);
    ShaderManager::activate(Shaders::PostProcess);

    auto downscaledMainTex = TextureManager::getRenderTexture(TextureID::Radiance);
    SetShaderValueTexture(ShaderManager::get(Shaders::PostProcess), mMainTextureUniform, downscaledMainTex.texture);
    gfx::DrawRenderTexture(TextureManager::getRenderTexture(TextureID::DownscaledPostProcess));
    EndShaderMode();
    EndTextureMode();
    // /2.

    // 3. Upscale the effects to the Staging buffer with additive blending
    auto fullResTex = TextureManager::getRenderTexture(TextureID::Staging);
    SetTextureFilter(effectsTarget.texture, TEXTURE_FILTER_POINT);
    BeginTextureMode(fullResTex);
    BeginBlendMode(BLEND_ADDITIVE);

    auto srcRect = Rectangle(0, 0, effectsTarget.texture.width, -effectsTarget.texture.height);
    auto dstRect = Rectangle(0, 0, fullResTex.texture.width, fullResTex.texture.height);
    DrawTexturePro(effectsTarget.texture, srcRect, dstRect, Vector2{0, 0}, 0.0f, WHITE);

    EndBlendMode();
    EndTextureMode();
    // /3.
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
    for (ecs::IRender* renderSystem : World.getRenderSystems()) {
        renderSystem->addToQueue(tmpDrawList);
        for (const gfx::EntityRenderInfo& renderInfo : tmpDrawList) {
            // Filter out hidden entities and entities outside of the viewport
            if (!renderInfo.entity.has<Invisible>() && cameraViewBox.isOverlapping(renderInfo.boundingBox)) {
                if (renderInfo.preciseTransform.depth == Depth::Debug || renderInfo.preciseTransform.depth == Depth::UIFar ||
                    renderInfo.preciseTransform.depth == Depth::UIClose) {
                    mUIRenderQueue.emplace_back(renderInfo);
                } else {
                    mRenderQueue.emplace_back(renderInfo);
                }
            }
        }

        tmpDrawList.clear();
    }

    std::sort(mRenderQueue.begin(), mRenderQueue.end(), isBelow);
    std::sort(mUIRenderQueue.begin(), mUIRenderQueue.end(), isBelow);
}

Color getPostProcessFlags(gfx::EntityRenderInfo renderInfo) {
    u8 r = 0, g = 0, b = 0;
    if (renderInfo.entity.has<GfxFlags>()) {
        u32 flags = renderInfo.entity.get<GfxFlags>().flags;
        if ((flags & GfxFlags::Bloom) > 0) {
            r = 255;
        }
        if ((flags & GfxFlags::Glow) > 0) {
            g = 255;
        }
    }

    // if (renderInfo.entity.has<BlocksLight>()) {
    //     b = 255;
    // }

    return Color(r, g, b, 255);
}

}  // namespace whal
