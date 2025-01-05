#include "Renderer.h"

#include <algorithm>
#include <raylib.h>
#include "Gfx/ShaderManager.h"
#include "Gfx/Shaders/DistanceField.h"
#include "Gfx/Shaders/Posterize.h"
#include "raylib/src/rlgl.h"
#include "whalECS/src/ECS.h"

#include "Gfx/RaylibUtil.h"
#include "Gfx/Shader.h"
#include "Gfx/Texture.h"

#include "Components/Camera.h"
#include "Components/Transform.h"

#include "Settings.h"
#include "Sys/System.h"

#include "Systems/ColliderSystem.h"
#include "Systems/Graphics/Common.h"
#include "Systems/LightSystem.h"

#include "Util/CameraUtil.h"
#include "Util/Print.h"

#include "Gfx/Shaders/LightDenoise.h"

namespace whal {

namespace gfx {

void applyShaders(rl::RenderTexture target, std::vector<std::shared_ptr<IShaderProcess>>& shaders) {
    rl::RenderTexture swap = Graphics.getTemporaryRT(target.texture);
    bool isSwapTarget = true;
    for (std::shared_ptr<IShaderProcess>& pShader : shaders) {
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
    mRaylibCamera.offset = rl::Vector2(WINDOW_WIDTH_RENDER / 2, WINDOW_HEIGHT_RENDER / 2);
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

void Renderer::blit(rl::RenderTexture src, rl::RenderTexture dst, rl::Shader shader) {
    const rl::Rectangle srcRect = rl::Rectangle(0, 0, src.texture.width, -src.texture.height);
    const rl::Rectangle dstRect = rl::Rectangle(0, 0, dst.texture.width, dst.texture.height);

    rl::BeginTextureMode(dst);
    rl::ClearBackground(Colors::ClearRL);
    const bool isCustomShader = shader.id != 0;
    if (mIsFixedShaderMode) {
        setUniforms(mFixedShader);
        rl::DrawTexturePro(src.texture, srcRect, dstRect, rl::Vector2{0, 0}, 0.0f, rl::WHITE);

    } else if (isCustomShader) {
        rl::BeginShaderMode(shader);
        setUniforms(shader);
        rl::DrawTexturePro(src.texture, srcRect, dstRect, rl::Vector2{0, 0}, 0.0f, rl::WHITE);
        rl::EndShaderMode();

    } else {
        rl::DrawTexturePro(src.texture, srcRect, dstRect, rl::Vector2{0, 0}, 0.0f, rl::WHITE);
    }
    rl::EndTextureMode();
}

void Renderer::render() {
    // 0. Create render context and build the render queue.
    rl::Camera2D worldCamera = mRaylibCamera;
    ecs::Entity cameraEntity = *getCamera();
    worldCamera.rotation = cameraEntity.get<Transform>().rotation;

    // dumb shit (raylib rounding issue that affects UVs when camera is exactly between 2 pixels in screen space)
    Vector2f cameraPosition = cameraEntity.get<Transform>().position;
    f32 decimal = math::abs(math::remainder(cameraPosition.y * VIRTUAL_SCREEN_RATIO));
    if (math::isNearZero(decimal - 0.5f, 0.005)) {
        cameraPosition.y += 0.01f * VIRTUAL_SCREEN_RATIO;
    }

    worldCamera.target = (cameraPosition * Vector2f(VIRTUAL_SCREEN_RATIO, -VIRTUAL_SCREEN_RATIO)).asRL();

    const gfx::RenderContext renderContext{
        .cameraPosition = cameraPosition,
        .camera = worldCamera,
        .atlas = TextureManager::getAtlas(TEXNAME_SPRITE),
        .cameraEntity = cameraEntity,
    };
    buildRenderQueue(renderContext.cameraPosition.round());

    // 1. IRender and IRenderLight systems are drawn
    drawEntities(renderContext);  // drawn to TextureID::Staging

    // camera drawn at different resolution, so gotta change camera stuff
    gfx::RenderContext lightRenderContext = renderContext;
    lightRenderContext.camera.target = (cameraPosition * Vector2f(1, -1)).asRL();
    lightRenderContext.camera.offset = rl::Vector2(WINDOW_WIDTH_GAME / 2, WINDOW_HEIGHT_GAME / 2);
    lightRenderContext.cameraPosition = lightRenderContext.camera.target;
    drawLights(lightRenderContext);  // drawn to TextureID::Lighting

    // posterize before applying lighting
    // TODO should belong to a pre-lighting postprocess pass in camera
    static Posterize sPosterize;
    auto tmpTex = getTemporaryRT(mStagingTexture.tex.texture);
    blit(mStagingTexture.tex, tmpTex);
    sPosterize.process(tmpTex, mStagingTexture.tex);
    releaseTemporaryRT(tmpTex);

    // 2. Renders everything to TextureID::Main
    rl::RenderTexture mainTex = TextureManager::getRenderTexture(TextureID::Main);
    rl::BeginTextureMode(mainTex);
    rl::ClearBackground(Colors::ClearRL);

    // Game Objects.
    gfx::DrawRenderTexture(mStagingTexture.tex);

    // Lights.
    rl::BeginBlendMode(rl::BLEND_MULTIPLIED);
    gfx::DrawRenderTexture(TextureManager::getRenderTexture(TextureID::Lighting));
    rl::EndBlendMode();

    // UI.
    drawUI(renderContext);
    rl::EndTextureMode();

    // 3. Apply post processing
    gfx::applyShaders(mainTex, cameraEntity.get<Camera>().postEffects);

    // 4. Draw debug stuff.
#ifndef NDEBUG
    if (Input.isHeld("view colliders")) {
        rl::BeginTextureMode(mainTex);
        rl::BeginMode2D(worldCamera);
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
    rl::ClearBackground(Colors::ClearRL);
    rl::DrawTexturePro(depthTex, srcRect, dstRect, rl::Vector2{0, 0}, 0.0f, rl::WHITE);
    rl::EndTextureMode();

    rl::BeginTextureMode(targetOcclDepthTex);
    rl::ClearBackground(Colors::ClearRL);
    rl::DrawTexturePro(occlDepthTex, srcRect, dstRect, rl::Vector2{0, 0}, 0.0f, rl::WHITE);
    rl::EndTextureMode();

    rl::BeginTextureMode(targetColorTex);
    rl::ClearBackground(Colors::ClearRL);
    rl::DrawTexturePro(colorTex, srcRect, dstRect, rl::Vector2{0, 0}, 0.0f, rl::WHITE);
    rl::EndTextureMode();
}

void Renderer::buildDistanceField() const {
    static DistanceField dfShader;
    const rl::RenderTexture occlSrc = TextureManager::getRenderTexture(TextureID::OcclusionColor);
    const rl::RenderTexture dfDst = TextureManager::getRenderTexture(TextureID::DistanceField);
    dfShader.process(occlSrc, dfDst);
}

// this does what the old Mega-GraphicsSystem used to do.
void Renderer::drawEntities(gfx::RenderContext renderContext) {
    // Drawing GAME OBJECTS
    rl::BeginTextureMode(mStagingTexture.tex);
    rl::ClearBackground(Colors::ClearRL);
    rl::BeginMode2D(renderContext.camera);
    const rl::Shader defaultShader = ShaderManager::get(Shaders::Default);
    for (const auto& renderInfo : mRenderQueue.mNormalQueue) {
        // Make sure we're using the default shader before each entity is drawn.
        // Shader swaps only happen if the new shader isn't the active one.
        // So this should be free on average.
        BeginShaderMode(defaultShader);
        renderInfo.piRender->draw(renderInfo, renderContext);
    }
    rl::EndMode2D();
    rl::EndTextureMode();

    scaleDepthBuffers(renderContext);
    buildDistanceField();
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
    for (auto renderInfo : mRenderQueue.mUIQueue) {
        renderInfo.piRender->draw(renderInfo, ctx);
    }
    rl::EndMode2D();
}

static bool isBelow(const gfx::EntityRenderInfo& entity1, const gfx::EntityRenderInfo& entity2) {
    if (entity1.transform.depth != entity2.transform.depth) {
        return entity1.transform.depth < entity2.transform.depth;
    }

    if constexpr (WORLD_TYPE == WorldType2D::TopDown) {
        return entity1.bottom > entity2.bottom;
    } else {
        return false;  // doesn't really matter
    }
}

void Renderer::buildRenderQueue(Vector2i cameraPosition) {
    // .clear() doesn't affect capacity
    mRenderQueue.clear();

    // Configure camera view box for culling
    const AABB cameraViewBox(cameraPosition, {WINDOW_WIDTH_GAME / 2 + PIXELS_PER_TILE, WINDOW_HEIGHT_GAME / 2 + PIXELS_PER_TILE});
    mRenderQueue.setViewBox(cameraViewBox);

    for (const ecs::RenderSystemPair& renderSystem : World.getRenderSystems()) {
        mRenderQueue.setActiveRenderer(renderSystem.pIRender);
        renderSystem.pIRender->addToQueue(mRenderQueue);
    }

    std::sort(mRenderQueue.mNormalQueue.begin(), mRenderQueue.mNormalQueue.end(), isBelow);
    std::sort(mRenderQueue.mUIQueue.begin(), mRenderQueue.mUIQueue.end(), isBelow);
}

void Renderer::queueUniform(UniformVariant uniform) {
    mUniformQueue.push_back(uniform);
}

void Renderer::setUniforms(rl::Shader shader) {
    if (mUniformQueue.size() == 0) {
        return;
    }

    for (auto uniform : mUniformQueue) {
        uniform.set(shader);
    }

    // if FixedShaderMode is activated and IsPersistUniforms is set, queue should stay the same. Clear otherwise.
    if (!(mIsFixedShaderMode && mIsPersistUniforms)) {
        mUniformQueue.clear();
    }
}

void Renderer::fixedShaderMode(rl::Shader shader, bool isPersistUniforms) {
    mIsFixedShaderMode = true;
    mIsPersistUniforms = isPersistUniforms;
    mFixedShader = shader;
    rl::BeginShaderMode(mFixedShader);
}

void Renderer::endFixedShaderMode() {
    mIsFixedShaderMode = false;
    rl::EndShaderMode();
    if (mIsPersistUniforms) {
        mUniformQueue.clear();
        mIsPersistUniforms = false;
    }
}

void UniformVariant::set(rl::Shader handle) const {
    switch (tag) {
    case UniformType::Float:
        rl::SetShaderValue(handle, uniformLoc, &val.uniFloat, rl::SHADER_UNIFORM_FLOAT);
        break;
    case UniformType::Vec2:
        rl::SetShaderValue(handle, uniformLoc, &val.uniVec2, rl::SHADER_UNIFORM_VEC2);
        break;
    case UniformType::Vec3:
        rl::SetShaderValue(handle, uniformLoc, &val.uniVec3, rl::SHADER_UNIFORM_VEC3);
        break;
    case UniformType::Vec4:
        rl::SetShaderValue(handle, uniformLoc, &val.uniVec4, rl::SHADER_UNIFORM_VEC4);
        break;
    case UniformType::Int:
        rl::SetShaderValue(handle, uniformLoc, &val.uniInt, rl::SHADER_UNIFORM_INT);
        break;
    case UniformType::Vec2i:
        rl::SetShaderValue(handle, uniformLoc, &val.uniVec2i, rl::SHADER_UNIFORM_IVEC2);
        break;
    case UniformType::Vec3i:
        rl::SetShaderValue(handle, uniformLoc, &val.uniVec3i, rl::SHADER_UNIFORM_IVEC3);
        break;
    case UniformType::Vec4i:
        rl::SetShaderValue(handle, uniformLoc, &val.uniVec4i, rl::SHADER_UNIFORM_IVEC4);
        break;
    case UniformType::Texture:
        rl::SetShaderValueTexture(handle, uniformLoc, val.uniTex);
        break;
    }
}

}  // namespace whal
