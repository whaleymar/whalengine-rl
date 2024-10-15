#include "Renderer.h"

#include <algorithm>
#include <raylib.h>
#include "raylib/src/rlgl.h"

#include "Components/GfxFlags.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Gfx/Pipeline.h"
#include "Gfx/Texture.h"

#include "Settings.h"
#include "Sys/System.h"

#include "Systems/CollisionManager.h"
#include "Systems/LightSystem.h"

#include "Systems/TagTrackers.h"
#include "Util/EngineUtil.h"
#include "whalECS/src/ECS.h"

namespace whal {

static s32 mainTexUniform;
static Color getPostProcessFlags(EntityRenderInfo renderInfo);

Renderer::Renderer() {
    mRaylibCamera.target = Vector2(0.0f, 0.0f);
    mRaylibCamera.zoom = 1.0f;
    mRaylibCamera.rotation = 0.0f;
}

void Renderer::render() {
    instance()._render();
}

void Renderer::setPostEffects(Pipeline pipeline) {
    instance().mPostProcessSteps = std::move(pipeline);
}

void Renderer::onEvent(ShaderReloadEvent) {
    mainTexUniform = GetShaderLocation(ShaderManager::get(Shaders::PostProcess), "iMainTex");
}

void Renderer::_render() {
    // 0. Create render context and build the render queue.
    Camera2D worldCamera = mRaylibCamera;
    ecs::Entity cameraEntity = *getCamera();
    worldCamera.rotation = cameraEntity.get<Transform2D>().rotationDegrees;
    const RenderContext renderContext{
        .cameraPosition = cameraEntity.get<PrecisePosition>().position, .camera = worldCamera, .atlas = TextureManager::getAtlas(TEXNAME_SPRITE)};
    buildRenderQueue(renderContext.cameraPosition.round());

    // 1. ECS systems with draw-like methods are updated (this should probably happen automatically)
    TextureManager::instance().renderBackgroundTextures();                      // drawn to TextureID::Background
    _drawEntities(renderContext);                                               // drawn to TextureID::Staging
    drawLights(worldCamera);                                                    // drawn to TextureID::UpscaledLighting
    System::world.getSystem<RadianceLightSystem>()->drawEntities(worldCamera);  // drawn to TextureID::Radiance

    // 2. Renders everything to TextureID::Main
    BeginTextureMode(TextureManager::getRenderTexture(TextureID::Main));
    ClearBackground(Colors::Clear);

    // Game Objects.
    drawRenderTexture(TextureManager::getRenderTexture(TextureID::Staging));

    // Lights.
    BeginBlendMode(BLEND_MULTIPLIED);
    drawRenderTexture(TextureManager::getRenderTexture(TextureID::UpscaledLighting));
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
    mPostProcessSteps.process(TextureID::Main);

    // 4. Draw debug stuff.
#ifndef NDEBUG
    if (System::input.isOn(InputType::DEBUG)) {
        BeginTextureMode(TextureManager::getRenderTexture(TextureID::Main));
        BeginMode2D(worldCamera);
        // System::world.getSystem<DrawDebugSystem>()->drawEntities();
        drawColliders();
        EndMode2D();
        EndTextureMode();
    }
#endif
}

// this does what the old Mega-GraphicsSystem used to do.
void Renderer::_drawEntities(RenderContext renderContext) const {
    // Draw to Effects Buffer (using main texture for this as it's unused at this point in the render pipeline)
    constexpr Color NO_EFFECT = Color{0, 0, 0, 0};
    BeginTextureMode(TextureManager::getRenderTexture(TextureID::Main));
    ClearBackground(NO_EFFECT);
    BeginMode2D(renderContext.camera);
    ShaderManager::activate(Shaders::Silhouette);
    for (auto renderInfo : mRenderQueue) {
        if (renderInfo.piRender->isPostProcessingUsed()) {
            const Color flags = getPostProcessFlags(renderInfo);
            renderContext.colorOverride = flags;
            renderInfo.piRender->draw(renderInfo.entity, renderContext);
        }
    }
    EndShaderMode();
    EndMode2D();
    EndTextureMode();
    renderContext.colorOverride = Corrade::Containers::NullOpt;

    // Draw downscaled version of the post-process texture
    // makes it much faster since the PP shader is SLOW.

    auto fullResTex = TextureManager::getRenderTexture(TextureID::Main);
    auto downscaledTarget = TextureManager::getRenderTexture(TextureID::DownscaledPostProcess);
    Rectangle srcRect = Rectangle(0, 0, fullResTex.texture.width, -fullResTex.texture.height);
    Rectangle dstRect = Rectangle(0, 0, downscaledTarget.texture.width, downscaledTarget.texture.height);

    BeginTextureMode(downscaledTarget);
    ClearBackground(NO_EFFECT);
    DrawTexturePro(fullResTex.texture, srcRect, dstRect, Vector2{0, 0}, 0.0f, WHITE);

    EndTextureMode();
    // TextureID::Main is now free to use

    // Drawing GAME OBJECTS
    BeginTextureMode(TextureManager::getRenderTexture(TextureID::Staging));
    ClearBackground(Colors::Clear);
    BeginMode2D(renderContext.camera);
    ShaderManager::activate(Shaders::Default);
    for (auto renderInfo : mRenderQueue) {
        renderInfo.piRender->draw(renderInfo.entity, renderContext);
    }
    EndShaderMode();
    EndMode2D();
    EndTextureMode();

    // Apply Post Processing Effects
    // Lighting and Radiance textures are unused at this point, so I use them as a temporary downscaled render target

    // 1. draw downscaled version of Staging
    fullResTex = TextureManager::getRenderTexture(TextureID::Staging);
    downscaledTarget = TextureManager::getRenderTexture(TextureID::Radiance);
    RenderTexture effectsTarget = TextureManager::getRenderTexture(TextureID::Lighting);

    BeginTextureMode(downscaledTarget);
    ClearBackground(Colors::Clear);
    DrawTexturePro(fullResTex.texture, srcRect, dstRect, Vector2{0, 0}, 0.0f, WHITE);
    EndTextureMode();
    // /1.

    // 2. Render effects to new buffer
    BeginTextureMode(effectsTarget);
    ClearBackground(Colors::Clear);
    ShaderManager::activate(Shaders::PostProcess);

    SetShaderValueTexture(ShaderManager::get(Shaders::PostProcess), mainTexUniform, downscaledTarget.texture);
    drawRenderTexture(TextureManager::getRenderTexture(TextureID::DownscaledPostProcess));
    EndShaderMode();
    EndTextureMode();
    // /2.

    // 3. Upscale the effects to the Staging buffer with additive blending
    SetTextureFilter(effectsTarget.texture, TEXTURE_FILTER_POINT);
    BeginTextureMode(fullResTex);
    BeginBlendMode(BLEND_ADDITIVE);
    // this... isn't working like it did before, but regular additive blend seems to look good
    // rlSetBlendFactorsSeparate(1, 1, 1, 1, 0x8006, 0x8007);
    // BeginBlendMode(BLEND_CUSTOM_SEPARATE);

    srcRect = Rectangle(0, 0, effectsTarget.texture.width, -effectsTarget.texture.height);
    dstRect = Rectangle(0, 0, fullResTex.texture.width, fullResTex.texture.height);
    DrawTexturePro(effectsTarget.texture, srcRect, dstRect, Vector2{0, 0}, 0.0f, WHITE);

    EndBlendMode();
    EndTextureMode();
    // /3.
}

void Renderer::_drawUI(const RenderContext ctx) const {
    BeginMode2D(ctx.camera);
    for (auto renderInfo : mUIRenderQueue) {
        renderInfo.piRender->draw(renderInfo.entity, ctx);
    }
    EndMode2D();
}

static bool isBelow(const EntityRenderInfo& entity1, const EntityRenderInfo& entity2) {
    if (entity1.depth != entity2.depth) {
        return entity1.depth < entity2.depth;
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
    static std::vector<EntityRenderInfo> tmpDrawList;  // make it static to minimize memory allocations per frame
    tmpDrawList.clear();

    const AABB cameraViewBox(cameraPosition, {WINDOW_WIDTH_GAME / 2, WINDOW_HEIGHT_GAME / 2});
    for (ecs::IRender* renderSystem : System::world.getRenderSystems()) {
        renderSystem->addToQueue(tmpDrawList);
        for (const EntityRenderInfo& renderInfo : tmpDrawList) {
            // Filter out hidden entities and entities outside of the viewport
            if (!renderInfo.entity.has<Invisible>() && cameraViewBox.isOverlapping(renderInfo.boundingBox)) {
                if (renderInfo.depth == Depth::Debug) {
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

Color getPostProcessFlags(EntityRenderInfo renderInfo) {
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

    if (renderInfo.entity.has<BlocksLight>()) {
        b = 255;
    }

    return Color(r, g, b, 255);
}

//
// RELIC: some tone mapping code I wrote for radiance:
//
// static int exposureUniform = GetShaderLocation(ShaderManager::get(Shaders::ToneMap), "exposure");
// static float exposure = 1.0;
//
// if (IsKeyPressed(KEY_UP)) {
//     exposure += 0.1;
//     print("exposure: ", exposure);
// } else if (IsKeyPressed(KEY_DOWN)) {
//     exposure -= 0.1;
//     print("exposure: ", exposure);
// }

// <draw radiance texture>

// auto shader = ShaderManager::get(Shaders::ToneMap);
// BeginShaderMode(shader);
// SetShaderValue(shader, exposureUniform, &exposure, SHADER_UNIFORM_FLOAT);
// DrawTexture(getRenderTexture(TextureID::Main).texture, 0, 0, WHITE);
// EndShaderMode();

}  // namespace whal
