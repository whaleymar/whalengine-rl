#include "Renderer.h"

#include <algorithm>
#include <raylib.h>

#include "Events/Events.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Shaders/DistanceField.h"
#include "Sys/Time.h"
#include "Util/ImguiUtil.h"
#include "raylib/src/rlgl.h"
#include "whalECS/src/ECS.h"

#include "Gfx/RaylibUtil.h"
#include "Gfx/Shader.h"
#include "Gfx/Texture.h"

#include "Components/Camera.h"
#include "Components/Transform.h"

#include "Settings.h"
#include "Sys/System.h"

#include "Systems/Graphics/Common.h"

#include "Util/CameraUtil.h"
#include "Util/Print.h"

namespace whal {

namespace gfx {

void applyShaders(rl::RenderTexture target, std::vector<IShaderProcess*>& shaders) {
    rl::RenderTexture swap = Graphics.getTemporaryRT(target.texture);
    bool isSwapTarget = true;
    for (IShaderProcess* pShader : shaders) {
        if (isSwapTarget) {
            pShader->process(target, swap);
        } else {
            pShader->process(swap, target);
        }
        isSwapTarget = !isSwapTarget;
    }

    if (!isSwapTarget) {
        // make sure final image is on the target
        Graphics.blit(swap, target, {0, nullptr}, rl::BLEND_ALPHA_PREMULTIPLY);
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

#define CHECK_ERROR(err)                                                                                                                             \
    if (err) {                                                                                                                                       \
        print(*err);                                                                                                                                 \
        return true;                                                                                                                                 \
    }

static bool loadTextureManagerDefaults() {
    auto err = TextureManager::instance().loadAndRegisterAtlas(SPRITE_TEXTURE_PATH, ATLAS_METADATA_PATH, TEXNAME_SPRITE);
    CHECK_ERROR(err);
    err = TextureManager::instance().loadAndRegister(NOISE_TEXTURE_PATH, "perlin_noise");
    CHECK_ERROR(err);
    err = TextureManager::instance().loadAndRegister(PALETTE_TEXTURE_PATH, TEXNAME_PALETTE);
    CHECK_ERROR(err);

    return false;
}

bool Renderer::init() {
    // Load default textures
    if (loadTextureManagerDefaults()) {
        return true;
    }

    mStagingTexture = new MultiTexture(MultiTexture::create(WINDOW_WIDTH_RENDER, WINDOW_HEIGHT_RENDER, rl::PIXELFORMAT_UNCOMPRESSED_R16G16B16A16));
    mGIOccluderTexture = new MultiTexture(MultiTexture::create(WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME, rl::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8));

    globalUniformRegister("_Time", UniformVariant{
                                       .tag = UniformVariant::Float,
                                       .val = {.uniFloat = 0.0f},
                                   });
    globalUniformRegister("_Resolution", UniformVariant{
                                             .tag = UniformVariant::Vec2,
                                             .val = {.uniVec2 = {FWINDOW_WIDTH_STRETCH, FWINDOW_HEIGHT_STRETCH}},
                                         });
    globalUniformRegister("_VirtualRatio", UniformVariant{
                                               .tag = UniformVariant::Float,
                                               .val = {.uniFloat = VIRTUAL_SCREEN_RATIO_STRETCH},
                                           });

    rl::Texture noiseTex = TextureManager::instance().getTexture("perlin_noise");
    globalUniformRegister("_PerlinNoise", UniformVariant{
                                              .tag = UniformVariant::Texture,
                                              .val = {.uniTex = noiseTex.id},
                                          });
    globalUniformRegister("_PerlinNoiseSize", UniformVariant{
                                                  UniformVariant::Vec2,
                                                  {.uniVec2 = rl::Vector2(noiseTex.width, noiseTex.height)},
                                              });

    return false;
}

void Renderer::end() {
    TextureManager::instance().unloadAll();
    mStagingTexture->release();
    mGIOccluderTexture->release();
    delete mStagingTexture;
    delete mGIOccluderTexture;
}

void Renderer::reset() {
    TextureManager::instance().unloadAll();
    loadTextureManagerDefaults();

    // global textures must be reloaded here
    rl::Texture noiseTex = TextureManager::instance().getTexture("perlin_noise");
    globalUniformSetTexture("_PerlinNoise", noiseTex);
    globalUniformSetVec2("_PerlinNoiseSize", rl::Vector2(noiseTex.width, noiseTex.height));
}

void Renderer::update() {
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

// If the OS window was resized, we need to update the global screen size variables
#ifdef __EMSCRIPTEN__
    static bool isFirstPass = true;
    static bool wasFullScreen = false;
    bool isFullScreen = rl::IsWindowFullscreen();
    // IsWindowResized never returns true on web
    if (isFirstPass || WINDOW_WIDTH_OS != rl::GetRenderWidth() || WINDOW_HEIGHT_OS != rl::GetRenderHeight() || isFullScreen != wasFullScreen) {
        isFirstPass = false;
        wasFullScreen = isFullScreen;
        // extra little hack to make sure the window is always in the right spot
        rl::SetWindowSize(rl::GetRenderWidth(), rl::GetRenderHeight());
        rl::SetWindowPosition(rl::GetWindowPosition().x, rl::GetWindowPosition().y);
#else
    if (rl::IsWindowResized()) {
#endif
        Vector2i newWindowSize(rl::GetRenderWidth(), rl::GetRenderHeight());
        if (newWindowSize.x == 0 || newWindowSize.y == 0) {
            // happens sometimes when fullscreening
            return;
        }
        updateWindowSizes(newWindowSize, newWindowSize);
        WINDOW_WIDTH_OS = newWindowSize.x;
        WINDOW_HEIGHT_OS = newWindowSize.y;

        // if we're in editor mode, this will force the dock window to recalculate
        WINDOW_WIDTH_DOCK = WINDOW_WIDTH_OS;
        WINDOW_HEIGHT_DOCK = WINDOW_HEIGHT_OS;
    }

    // update global uniforms that the Renderer owns
    globalUniformSetFloat("_Time", Time.getElapsed());
}

rl::RenderTexture Renderer::getTemporaryRT(s32 width, s32 height, rl::PixelFormat format, rl::TextureFilter filter, rl::TextureWrap wrap) {
    // iterate backwards, since the most recently used stuff is in the back
    for (s32 i = static_cast<s32>(mAvailableRTs.size()) - 1; i >= 0; --i) {
        auto& it = mAvailableRTs[i];
        // if the filter/wrap don't match that's fine, we can just change it
        if (it.rt.texture.width == width && it.rt.texture.height == height && it.rt.texture.format == format) {
            // move this to mUsedRTs and return it
            auto result = mAvailableRTs[i];
            mAvailableRTs.erase(mAvailableRTs.begin() + i);
            if (result.filter != filter) {
                result.filter = filter;
                rl::SetTextureFilter(result.rt.texture, filter);
            }
            if (result.wrap != wrap) {
                result.wrap = wrap;
                rl::SetTextureWrap(result.rt.texture, wrap);
            }
            mUsedRTs.push_back(result);
            return result.rt;
        }
    }

    // Nothing was found, create a new RenderTexture
    rl::RenderTexture rt = rl::LoadRenderTextureFormat(width, height, format);
    rl::SetTextureFilter(rt.texture, filter);
    if (wrap != rl::TEXTURE_WRAP_REPEAT) {
        rl::SetTextureWrap(rt.texture, wrap);
    }
    mUsedRTs.push_back({.rt = rt, .filter = filter, .wrap = wrap});
    return rt;
}

rl::RenderTexture Renderer::getTemporaryRT(rl::Texture reference, rl::TextureFilter filter, rl::TextureWrap wrap) {
    return getTemporaryRT(reference.width, reference.height, static_cast<rl::PixelFormat>(reference.format), filter, wrap);
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

void Renderer::blit(rl::RenderTexture src, rl::RenderTexture dst, rl::Shader shader, rl::BlendMode blendMode) {
    const rl::Rectangle srcRect = rl::Rectangle(0, 0, src.texture.width, -src.texture.height);
    const rl::Rectangle dstRect = rl::Rectangle(0, 0, dst.texture.width, dst.texture.height);

    gfx::BeginTextureMode(dst);
    rl::ClearBackground(Colors::ClearRL);
    if (blendMode != rl::BLEND_ALPHA) {
        rl::BeginBlendMode(blendMode);
    }

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

    if (blendMode != rl::BLEND_ALPHA) {
        rl::EndBlendMode();
    }
    gfx::EndTextureMode();
}

gfx::RenderContext Renderer::getRenderContext(bool useUnstretchedRenderWindow) const {
    rl::Camera2D worldCamera = mRaylibCamera;
    const f32 vRatio = useUnstretchedRenderWindow ? VIRTUAL_SCREEN_RATIO : VIRTUAL_SCREEN_RATIO_STRETCH;
    if (!useUnstretchedRenderWindow) {
        worldCamera.offset = rl::Vector2(WINDOW_WIDTH_STRETCH / 2, WINDOW_HEIGHT_STRETCH / 2);
    }
    ecs::Entity cameraEntity = getCamera();
    worldCamera.rotation = cameraEntity.get<Transform>().rotation;

    // HACK dumb shit (raylib rounding issue that affects UVs when camera is exactly between 2 pixels in screen space)
    Vector2f cameraPosition = cameraEntity.get<Transform>().position;
    f32 decimal = math::abs(math::getDecimal(cameraPosition.y * vRatio));
    if (math::isNearZero(decimal - 0.5f, 0.005)) {
        cameraPosition.y += 0.01f;
    }

    worldCamera.target = (cameraPosition * Vector2f(vRatio, -vRatio)).asRL();

    return gfx::RenderContext{
        .cameraPosition = cameraPosition,
        .cameraViewHalf = {(WINDOW_WIDTH_GAME + PIXELS_PER_TILE) / 2, (WINDOW_HEIGHT_GAME + PIXELS_PER_TILE) / 2},
        .camera = worldCamera,
        .atlas = TextureManager::getAtlas(TEXNAME_SPRITE),
        .cameraEntity = cameraEntity,
    };
}

void Renderer::render() {
    // 0. Create render context and build the render queue.
    const gfx::RenderContext renderContext = getRenderContext();
    buildRenderQueue(renderContext.cameraPosition.round(), renderContext.cameraViewHalf);

    // 1. IRender and IRenderLight systems are drawn
    drawEntities(renderContext);  // drawn to TextureID::Staging

    // lights drawn at different resolution, so gotta change camera stuff
    gfx::RenderContext lightRenderContext = renderContext;
    lightRenderContext.camera.target = (renderContext.cameraPosition * Vector2f(1, -1)).asRL();
    lightRenderContext.camera.offset = rl::Vector2(WINDOW_WIDTH_GAME / 2, WINDOW_HEIGHT_GAME / 2);
    lightRenderContext.cameraPosition = renderContext.cameraPosition;
    drawLights(lightRenderContext);  // drawn to TextureID::Lighting

    Camera cameraSettings = renderContext.cameraEntity.get<Camera>();
    gfx::applyShaders(mStagingTexture->tex, cameraSettings.beforeLighting);

    // 2. Renders everything to TextureID::Main
    rl::RenderTexture mainTex = TextureManager::getRenderTexture(TextureID::Main);
    gfx::BeginTextureMode(mainTex);
    rl::ClearBackground(Colors::ClearRL);

    // Game Objects.
    // required for FBOs that will be drawn to the screen (?) Otherwise things with transparency look black:
    rl::BeginBlendMode(rl::BLEND_ALPHA_PREMULTIPLY);
    gfx::DrawRenderTexture(mStagingTexture->tex);
    rl::EndBlendMode();

    // Lights.
    rl::BeginBlendMode(rl::BLEND_MULTIPLIED);
    gfx::DrawRenderTexture(TextureManager::getRenderTexture(TextureID::Lighting));
    rl::EndBlendMode();

    // UI.
    drawUI(renderContext);
    gfx::EndTextureMode();

    // 3. Apply post processing
    gfx::applyShaders(mainTex, cameraSettings.postEffects);

    // 4. Draw debug stuff.
#ifndef NDEBUG
    gfx::BeginTextureMode(mainTex);
    rl::BeginMode2D(renderContext.camera);
    DebugRenderMgr::drawDebug();
    rl::EndMode2D();
    gfx::EndTextureMode();
#endif
}

void Renderer::buildDistanceField() const {
    const rl::RenderTexture occlSrc = TextureManager::getRenderTexture(TextureID::OcclusionDepth);
    const rl::RenderTexture dfDst = TextureManager::getRenderTexture(TextureID::DistanceField);
    DistanceField::instance().process(occlSrc, dfDst);
}

void Renderer::drawRenderQueue(const MultiTexture& target, const gfx::RenderContext& renderContext, const std::vector<gfx::EntityRenderInfo>& queue) {
    gfx::BeginTextureMode(target.tex);
    rl::ClearBackground(Colors::ClearRL);
    rl::BeginMode2D(renderContext.camera);
    const Shader* defaultShader = &ShaderMgr::get("DefaultSprite");
    const Shader* lastShader = nullptr;
    defaultShader->bind();
    for (const auto& renderInfo : queue) {
        if (renderInfo.shader != lastShader) {
            if (renderInfo.shader == nullptr) {
                defaultShader->bind();
            } else {
                renderInfo.shader->bind();
            }
            lastShader = renderInfo.shader == defaultShader ? nullptr : renderInfo.shader;
        }
        renderInfo.piRender->draw(renderInfo, renderContext);
    }
    rl::EndShaderMode();
    rl::EndMode2D();
    gfx::EndTextureMode();
}

void Renderer::drawEntities(gfx::RenderContext renderContext) {
    // Drawing GAME OBJECTS
    drawRenderQueue(*mStagingTexture, renderContext, mRenderQueue.mNormalQueue);

    // adjust the camera and virtual ratio to work with a game-resolution camera
    f32 prevVirtualRatio = VIRTUAL_SCREEN_RATIO;
    VIRTUAL_SCREEN_RATIO = 1.0f;  // HACK
    gfx::RenderContext gameRenderContext = renderContext;
    gameRenderContext.camera.offset = rl::Vector2(WINDOW_WIDTH_GAME / 2, WINDOW_HEIGHT_GAME / 2);
    gameRenderContext.isOccludersOnly = true;

    auto renderOccluders = [&](s32 sector, const std::vector<gfx::EntityRenderInfo>& queue) {
        Vector2f offset = gfx::getGISectorOffset(sector);
        // gameRenderContext.cameraPosition =
        Vector2f renderCameraPosition = renderContext.cameraPosition + offset;
        gameRenderContext.camera.target = (renderCameraPosition * Vector2f(1, -1)).asRL();
        const AABB giViewBox = gfx::getGIViewBox(renderCameraPosition.round(), sector);

        // TODO non-ysorted tilemap layers are drawing outside of their sector bounds but I can't figure out the math :(
        gameRenderContext.cameraPosition = renderCameraPosition;
        // gameRenderContext.cameraPosition = giViewBox.getPosition().as<f32>();
        gameRenderContext.cameraViewHalf = giViewBox.getHalf();

        drawRenderQueue(*mGIOccluderTexture, gameRenderContext, queue);
    };
    renderOccluders(8, mRenderQueue.mOccluderQueueCamera);

    // COPY CAMERA'S COLOR AND DEPTH BUFFERS FOR LATER
    const rl::Rectangle srcRect = rl::Rectangle(0, 0, WINDOW_WIDTH_GAME, -WINDOW_HEIGHT_GAME);
    const rl::Rectangle dstRect = rl::Rectangle(0, 0, WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME);

    gfx::BeginTextureMode(TextureManager::getRenderTexture(TextureID::OcclusionColor));
    rl::ClearBackground(Colors::ClearRL);
    rl::DrawTexturePro(mGIOccluderTexture->tex.texture, srcRect, dstRect, rl::Vector2{0, 0}, 0.0f, rl::WHITE);
    gfx::EndTextureMode();

    auto tmp = getTemporaryRT(WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME);
    gfx::BeginTextureMode(tmp);
    rl::ClearBackground(Colors::ClearRL);
    rl::DrawTexturePro(mGIOccluderTexture->getDepth(), srcRect, dstRect, rl::Vector2{0, 0}, 0.0f, rl::WHITE);
    gfx::EndTextureMode();

    s32 sector = Time.getFrame() % 8;
    renderOccluders(sector, mRenderQueue.mOccluderQueue);

    // TEMP TESTING
    // gfx::BeginTextureMode(TextureManager::getRenderTexture(TextureID::OcclusionColor));
    // rl::ClearBackground(rl::WHITE);
    // rl::DrawTexturePro(mGIOccluderTexture.getDepth(), srcRect, dstRect, rl::Vector2{0, 0}, 0.0f, rl::WHITE);
    // looking at cropped draw:
    // auto sectorSize = gfx::getGISectorSize(sector);
    // rl::DrawTexturePro(mGIOccluderTexture.getDepth(), rl::Rectangle(0.0, sectorSize.y, sectorSize.x, -sectorSize.y),
    //                    rl::Rectangle{0, 0, sectorSize.x, sectorSize.y}, rl::Vector2{0, 0}, 0, rl::WHITE);
    // gfx::EndTextureMode();

    VIRTUAL_SCREEN_RATIO = prevVirtualRatio;
    scaleDepthBuffers(renderContext, tmp.texture);
    releaseTemporaryRT(tmp);

    buildDistanceField();
}

void Renderer::drawLights(gfx::RenderContext renderContext) {
    rl::RenderTexture lightTex =
        Graphics.getTemporaryRT(WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME, rl::PIXELFORMAT_UNCOMPRESSED_R16G16B16A16, rl::TEXTURE_FILTER_BILINEAR);
    gfx::BeginTextureMode(lightTex);
    rl::BeginMode2D(renderContext.camera);
    rl::ClearBackground(rl::BLACK);

    rl::BeginBlendMode(rl::BLEND_ADDITIVE);
    for (const ecs::IRenderLight* pLightSystem : World.getLightSystems()) {
        pLightSystem->draw(renderContext);
    }

    rl::EndBlendMode();
    rl::EndMode2D();
    gfx::EndTextureMode();

    rl::RenderTexture lightTexUpscale = TextureManager::getRenderTexture(TextureID::Lighting);
    Camera cameraSettings = getCamera().get<Camera>();
    IShaderProcess* upscaleShader = cameraSettings.lightingUpscaler;
    if (upscaleShader) {
        upscaleShader->process(lightTex, lightTexUpscale);
    } else {
        // If I don't want blur, this shader just sets alpha to 1 for all values, otherwise multiplication gets weird
        blit(lightTex, lightTexUpscale, ShaderMgr::get("LightPassThrough").get());
    }
    Graphics.releaseTemporaryRT(lightTex);
}

void Renderer::scaleDepthBuffers(gfx::RenderContext ctx, rl::Texture cameraDepthView) const {
    // Downscale the Multi-Render Target buffers to Game resolution (for lighting)
    const auto allDepthTex = mStagingTexture->getDepth();

    const auto targetDepthTex = TextureManager::getRenderTexture(TextureID::Depth);
    rl::Rectangle srcRect = rl::Rectangle(0, 0, WINDOW_WIDTH_RENDER, -WINDOW_HEIGHT_RENDER);
    rl::Rectangle dstRect = rl::Rectangle(0, 0, WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME);

    gfx::BeginTextureMode(targetDepthTex);
    rl::ClearBackground(Colors::ClearRL);
    rl::DrawTexturePro(allDepthTex, srcRect, dstRect, rl::Vector2{0, 0}, 0.0f, rl::WHITE);
    gfx::EndTextureMode();

    // Render to larger occlusion color buf
    // clear the center sector and draw the occlusion stuff visible to the camera
    const rl::Rectangle srcRectGame = rl::Rectangle(0, 0, WINDOW_WIDTH_GAME, -WINDOW_HEIGHT_GAME);
    rl::RenderTexture globalOccl = TextureManager::getRenderTexture(TextureID::OcclusionDepth);
    gfx::BeginTextureMode(globalOccl);
    Vector2f loc = gfx::getGISector(8);
    rl::rlSetBlendFactors(RL_ONE, RL_ZERO, RL_FUNC_ADD);
    rl::BeginBlendMode(rl::BLEND_CUSTOM);
    rl::DrawRectangle(loc.x, loc.y, WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME, Colors::ClearRL);

    // draw camera sector
    rl::DrawTexturePro(cameraDepthView, srcRectGame, rl::Rectangle{loc.x, loc.y, FWINDOW_WIDTH_GAME, FWINDOW_HEIGHT_GAME}, rl::Vector2{0, 0}, 0,
                       rl::WHITE);

    s32 sector = Time.getFrame() % 8;
    loc = gfx::getGISector(sector);

    const Vector2f sectorSize = gfx::getGISectorSize(sector);
    // clear this sector:
    rl::DrawRectangle(loc.x, loc.y, sectorSize.x, sectorSize.y, Colors::ClearRL);

    // draw updated sector
    rl::DrawTexturePro(mGIOccluderTexture->getDepth(), rl::Rectangle(0.0, sectorSize.y, sectorSize.x, -sectorSize.y),
                       rl::Rectangle{loc.x, loc.y, sectorSize.x, sectorSize.y}, rl::Vector2{0, 0}, 0, rl::WHITE);
    rl::EndBlendMode();
    gfx::EndTextureMode();
}

void Renderer::drawUI(const gfx::RenderContext ctx) const {
    rl::BeginMode2D(ctx.camera);
    for (auto renderInfo : mRenderQueue.mUIQueue) {
        renderInfo.piRender->draw(renderInfo, ctx);
    }
    rl::EndMode2D();
}

// sort by depth, then y coord, then shader, then entity id
static bool isBelow(const gfx::EntityRenderInfo& entity1, const gfx::EntityRenderInfo& entity2) {
    if (entity1.colorBuf.depth != entity2.colorBuf.depth) {
        return entity1.colorBuf.depth < entity2.colorBuf.depth;
    }

    if constexpr (WORLD_TYPE == WorldType2D::TopDown) {
        if (entity1.ysortPosition != entity2.ysortPosition) {
            return entity1.ysortPosition > entity2.ysortPosition;
        }
    }

    if (entity1.shader != entity2.shader) {
        return entity1.shader < entity2.shader;
    }

    // final tie breaker: use entity id for consistency
    return entity1.entity.id() < entity2.entity.id();
}

void Renderer::buildRenderQueue(Vector2i cameraPosition, Vector2i cameraViewHalf) {
    mRenderQueue.clear();

    // Configure camera view boxes for culling
    const AABB cameraViewBox(cameraPosition, cameraViewHalf);
    mRenderQueue.setViewBox(cameraViewBox);

    s32 giSector = Time.getFrame() % 8;
    mRenderQueue.setGIViewBox(gfx::getGIViewBox(cameraPosition, giSector));

    for (const ecs::RenderSystemPair& renderSystem : World.getRenderSystems()) {
        mRenderQueue.setActiveRenderer(renderSystem.pIRender);
        renderSystem.pIRender->addToQueue(mRenderQueue);
    }

    std::sort(mRenderQueue.mNormalQueue.begin(), mRenderQueue.mNormalQueue.end(), isBelow);
    std::sort(mRenderQueue.mUIQueue.begin(), mRenderQueue.mUIQueue.end(), isBelow);
    std::sort(mRenderQueue.mOccluderQueue.begin(), mRenderQueue.mOccluderQueue.end(), isBelow);
    std::sort(mRenderQueue.mOccluderQueueCamera.begin(), mRenderQueue.mOccluderQueueCamera.end(), isBelow);
}

void Renderer::queueUniform(ShaderUniform uniform) {
    mUniformQueue.push_back(uniform);
}

void Renderer::setUniforms(rl::Shader shader) {
    // enable shader once (instead of doing it per-uniform like default raylib)
    // I think I still want this line to run even if all values are cached so the correct shader is bound
    rl::rlEnableShader(shader.id);

    globalUniformBindAll(shader);

    if (mUniformQueue.size() == 0) {
        return;
    }

    for (auto uniform : mUniformQueue) {
        uniform.value.set(shader, uniform.loc);
    }

    // if FixedShaderMode is activated and IsPersistUniforms is set, queue should stay the same. Clear otherwise.
    // RESEARCH is persisting the uniform queue necessary? Won't they stay constant?
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

void Renderer::updateWindowSizes(Vector2i renderSize, Vector2i parentSize, Vector2i windowPosition) {
    // Force the aspect ratio to be the same as the game:
    const f32 targetAR = FWINDOW_WIDTH_GAME / FWINDOW_HEIGHT_GAME;
    Vector2i newSize = renderSize;
    const f32 newAR = static_cast<f32>(newSize.x) / static_cast<f32>(newSize.y);
    if (newAR > targetAR) {
        // too wide
        // can we increase height?
        s32 maybe = std::round(newSize.x / targetAR);
        if (maybe <= parentSize.y) {
            newSize.y = maybe;
        } else {
            // can't get taller, so need to reduce width
            newSize.x = std::round(targetAR * newSize.y);
        }
    } else if (newAR < targetAR) {
        // too tall
        // can we increase width?
        s32 maybe = std::round(targetAR * newSize.y);
        if (maybe <= parentSize.x) {
            newSize.x = maybe;
        } else {
            // can't get wider, so need to reduce height
            newSize.y = std::round(newSize.x / targetAR);
        }
    }

    WINDOW_WIDTH_STRETCH = newSize.x;
    WINDOW_HEIGHT_STRETCH = newSize.y;

    // make sure we aren't exceeding the size limit
    if (IS_CAP_RENDER_WINDOW) {
        if (newSize.x > WINDOW_MAX_WIDTH_RENDER) {
            newSize.x = WINDOW_MAX_WIDTH_RENDER;
        }
        if (newSize.y > WINDOW_MAX_HEIGHT_RENDER) {
            newSize.y = WINDOW_MAX_HEIGHT_RENDER;
        }
    }
    Vector2f oldSize(FWINDOW_WIDTH_RENDER, FWINDOW_HEIGHT_RENDER);
    WINDOW_WIDTH_RENDER = newSize.x;
    WINDOW_HEIGHT_RENDER = newSize.y;
    cascadeWindowChanges(parentSize, windowPosition);

    mStagingTexture->release();
    delete mStagingTexture;
    mStagingTexture = new MultiTexture(MultiTexture::create(WINDOW_WIDTH_RENDER, WINDOW_HEIGHT_RENDER, rl::PIXELFORMAT_UNCOMPRESSED_R16G16B16A16));
    TextureManager::instance().reloadRenderTextures();
    mRaylibCamera.offset = rl::Vector2(WINDOW_WIDTH_RENDER / 2, WINDOW_HEIGHT_RENDER / 2);
    Event.emit<evt::WindowResize>(Vector2f(FWINDOW_WIDTH_RENDER / oldSize.x, FWINDOW_HEIGHT_RENDER / oldSize.y));
}

void Renderer::toggleFullscreen() {
    if (rl::IsWindowFullscreen()) {
        // deactivate:
        rl::ToggleFullscreen();
        rl::SetWindowSize(mPrevWindowSizeBeforeFullscreen.x, mPrevWindowSizeBeforeFullscreen.y);
        rl::SetWindowPosition(mPrevWindowPosBeforeFullscreen.x, mPrevWindowPosBeforeFullscreen.y);
    } else {
        // activate:
        mPrevWindowSizeBeforeFullscreen = {WINDOW_WIDTH_OS, WINDOW_HEIGHT_OS};
        mPrevWindowPosBeforeFullscreen = rl::GetWindowPosition();
        int monitor = rl::GetCurrentMonitor();
        rl::SetWindowSize(rl::GetMonitorWidth(monitor), rl::GetMonitorHeight(monitor));
        rl::ToggleFullscreen();
    }
}

void Renderer::cascadeWindowChanges(Vector2i parentSize, Vector2i windowPosition) {
    if (windowPosition.x == -1) {
        // automatically calculate it
        if (parentSize.x != WINDOW_WIDTH_STRETCH) {
            WINDOW_POS_OS_X = (parentSize.x - WINDOW_WIDTH_STRETCH) / 2;
        } else {
            WINDOW_POS_OS_X = 0;
        }
    } else {
        WINDOW_POS_OS_X = windowPosition.x;
    }

    if (windowPosition.y == -1) {
        if (parentSize.y != WINDOW_HEIGHT_STRETCH) {
            WINDOW_POS_OS_Y = (parentSize.y - WINDOW_HEIGHT_STRETCH) / 2;
        } else {
            WINDOW_POS_OS_Y = 0;
        }

    } else {
        WINDOW_POS_OS_Y = windowPosition.y;
    }

    FWINDOW_WIDTH_RENDER = WINDOW_WIDTH_RENDER;
    FWINDOW_HEIGHT_RENDER = WINDOW_HEIGHT_RENDER;
    FWINDOW_WIDTH_GAME = WINDOW_WIDTH_GAME;
    FWINDOW_HEIGHT_GAME = WINDOW_HEIGHT_GAME;
    VIRTUAL_SCREEN_RATIO = FWINDOW_WIDTH_RENDER / FWINDOW_WIDTH_GAME;
    FWINDOW_WIDTH_STRETCH = WINDOW_WIDTH_STRETCH;
    FWINDOW_HEIGHT_STRETCH = WINDOW_HEIGHT_STRETCH;
    VIRTUAL_SCREEN_RATIO_STRETCH = FWINDOW_WIDTH_STRETCH / FWINDOW_WIDTH_GAME;

    // HACK
    // if VIRTUAL_SCREEN_RATIO * game_height has a decimal value of approx. 0.5, then we get artifacts
    // from floating point errors, so we need to slightly tweak the window size
    f32 decimal = math::abs(math::getDecimal(VIRTUAL_SCREEN_RATIO * FWINDOW_HEIGHT_GAME));
    if (math::isNearZero(decimal - 0.5f, 0.005)) {
        WINDOW_WIDTH_RENDER -= 1;
        WINDOW_HEIGHT_RENDER -= 1;
        cascadeWindowChanges(parentSize, windowPosition);
    }
}

void Renderer::globalUniformRegister(const std::string& name, UniformVariant initialValue) {
    assert(!mGlobalUniformNameToIndex.contains(name) && "Global uniform is already registered");
    u64 ix = mGlobalUniforms.size();
    mGlobalUniformNameToIndex[name] = ix;
    mGlobalUniforms.push_back(initialValue);
}

void Renderer::globalUniformSubscribe(const std::string& name, const Shader& shader) {
    assert(mGlobalUniformNameToIndex.contains(name) && "Global uniform was not registered!");
    u64 uniformIx = mGlobalUniformNameToIndex[name];

    // get the location of this uniform in the shader and add it as a subscriber
    s32 loc = rl::GetShaderLocation(shader.get(), name.c_str());
    if (loc == -1) {
        print(whal_format("Global uniform {} not found in shader {}", name, shader.getPath()));
        return;
    }

    u32 shaderId = shader.get().id;
    GlobalUniformTracker tracker = GlobalUniformTracker{
        .index = uniformIx,
        .uniformLoc = loc,
    };
    auto it = mGlobalUniformSubscribers.find(shaderId);
    if (it == mGlobalUniformSubscribers.end()) {
        mGlobalUniformSubscribers.insert({shaderId, {tracker}});
    } else {
        it->second.push_back(tracker);
    }
}

void Renderer::globalUniformBindAll(rl::Shader shader) {
    // RESEARCH figure out a way to not update uniforms that haven't changed
    auto it = mGlobalUniformSubscribers.find(shader.id);
    if (it == mGlobalUniformSubscribers.end()) {
        return;  // not subscribed to any global values
    }
    for (GlobalUniformTracker tracker : it->second) {
        mGlobalUniforms[tracker.index].set(shader, tracker.uniformLoc);
    }
}

void Renderer::globalUniformOnShaderUnload(rl::Shader shader) {
    auto it = mGlobalUniformSubscribers.find(shader.id);
    if (it != mGlobalUniformSubscribers.end()) {
        mGlobalUniformSubscribers.erase(it);
    }
}

void Renderer::globalUniformSetFloat(const std::string& name, f32 val) {
    auto it = mGlobalUniformNameToIndex.find(name);
    assert(it != mGlobalUniformNameToIndex.end() && "Setting value for unregistered global uniform");
    mGlobalUniforms[it->second].val.uniFloat = val;
}

void Renderer::globalUniformSetInt(const std::string& name, s32 val) {
    auto it = mGlobalUniformNameToIndex.find(name);
    assert(it != mGlobalUniformNameToIndex.end() && "Setting value for unregistered global uniform");
    mGlobalUniforms[it->second].val.uniInt = val;
}

void Renderer::globalUniformSetTexture(const std::string& name, rl::Texture val) {
    auto it = mGlobalUniformNameToIndex.find(name);
    assert(it != mGlobalUniformNameToIndex.end() && "Setting value for unregistered global uniform");
    mGlobalUniforms[it->second].val.uniTex = val.id;
}

void Renderer::globalUniformSetVec2(const std::string& name, Vector2f val) {
    auto it = mGlobalUniformNameToIndex.find(name);
    assert(it != mGlobalUniformNameToIndex.end() && "Setting value for unregistered global uniform");
    mGlobalUniforms[it->second].val.uniVec2 = val.asRL();
}

void Renderer::globalUniformSetVec2(const std::string& name, rl::Vector2 val) {
    auto it = mGlobalUniformNameToIndex.find(name);
    assert(it != mGlobalUniformNameToIndex.end() && "Setting value for unregistered global uniform");
    mGlobalUniforms[it->second].val.uniVec2 = val;
}

void Renderer::globalUniformSetVec3(const std::string& name, rl::Vector3 val) {
    auto it = mGlobalUniformNameToIndex.find(name);
    assert(it != mGlobalUniformNameToIndex.end() && "Setting value for unregistered global uniform");
    mGlobalUniforms[it->second].val.uniVec3 = val;
}

void Renderer::globalUniformSetVec4(const std::string& name, rl::Vector4 val) {
    auto it = mGlobalUniformNameToIndex.find(name);
    assert(it != mGlobalUniformNameToIndex.end() && "Setting value for unregistered global uniform");
    mGlobalUniforms[it->second].val.uniVec4 = val;
}

void UniformVariant::set(rl::Shader handle, s32 uniformLoc) const {
    switch (tag) {
    case Float:
        rl::rlSetUniform(uniformLoc, &val, rl::SHADER_UNIFORM_FLOAT, 1);
        break;
    case Vec2:
        rl::rlSetUniform(uniformLoc, &val, rl::SHADER_UNIFORM_VEC2, 1);
        break;
    case Vec3:
        rl::rlSetUniform(uniformLoc, &val.uniVec3, rl::SHADER_UNIFORM_VEC3, 1);
        break;
    case Vec4:
        rl::rlSetUniform(uniformLoc, &val.uniVec4, rl::SHADER_UNIFORM_VEC4, 1);
        break;
    case Int:
        rl::rlSetUniform(uniformLoc, &val.uniInt, rl::SHADER_UNIFORM_INT, 1);
        break;
    case Vec2i:
        rl::rlSetUniform(uniformLoc, &val.uniVec2i, rl::SHADER_UNIFORM_IVEC2, 1);
        break;
    case Vec3i:
        rl::rlSetUniform(uniformLoc, &val.uniVec3i, rl::SHADER_UNIFORM_IVEC3, 1);
        break;
    case Vec4i:
        rl::rlSetUniform(uniformLoc, &val.uniVec4i, rl::SHADER_UNIFORM_IVEC4, 1);
        break;
    case Texture:
        rl::rlSetUniformSampler(uniformLoc, val.uniTex);
        break;
    }
}

}  // namespace whal
