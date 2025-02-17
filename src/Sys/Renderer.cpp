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
    mStagingTexture = MultiTexture::create(WINDOW_WIDTH_RENDER, WINDOW_HEIGHT_RENDER, rl::PIXELFORMAT_UNCOMPRESSED_R16G16B16A16);
    mGIOccluderTexture = MultiTexture::create(WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME, rl::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
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
    if (rl::IsWindowResized()) {
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

gfx::RenderContext Renderer::getRenderContext(bool useUnstretchedRenderWindow) const {
    rl::Camera2D worldCamera = mRaylibCamera;
    const f32 vRatio = useUnstretchedRenderWindow ? VIRTUAL_SCREEN_RATIO : VIRTUAL_SCREEN_RATIO_STRETCH;
    if (!useUnstretchedRenderWindow) {
        worldCamera.offset = rl::Vector2(WINDOW_WIDTH_STRETCH / 2, WINDOW_HEIGHT_STRETCH / 2);
    }
    ecs::Entity cameraEntity = *getCamera();
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
        .camera = worldCamera,
        .atlas = TextureManager::getAtlas(TEXNAME_SPRITE),
        .cameraEntity = cameraEntity,
    };
}

void Renderer::render() {
    // 0. Create render context and build the render queue.
    const gfx::RenderContext renderContext = getRenderContext();
    buildRenderQueue(renderContext.cameraPosition.round());

    // 1. IRender and IRenderLight systems are drawn
    drawEntities(renderContext);  // drawn to TextureID::Staging

    // lights drawn at different resolution, so gotta change camera stuff
    gfx::RenderContext lightRenderContext = renderContext;
    lightRenderContext.camera.target = (renderContext.cameraPosition * Vector2f(1, -1)).asRL();
    lightRenderContext.camera.offset = rl::Vector2(WINDOW_WIDTH_GAME / 2, WINDOW_HEIGHT_GAME / 2);
    lightRenderContext.cameraPosition = renderContext.cameraPosition;
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
    gfx::applyShaders(mainTex, renderContext.cameraEntity.get<Camera>().postEffects);

    // 4. Draw debug stuff.
#ifndef NDEBUG
    if (VIEW_COLLIDERS_MODE) {
        rl::BeginTextureMode(mainTex);
        rl::BeginMode2D(renderContext.camera);
        drawColliders();
        rl::EndMode2D();
        rl::EndTextureMode();
    }
#endif
}

void Renderer::scaleDepthBuffers(gfx::RenderContext ctx, rl::Texture cameraDepthView) const {
    // Downscale the Multi-Render Target buffers to Game resolution (for lighting)
    const auto allDepthTex = mStagingTexture.getDepth();

    const auto targetDepthTex = TextureManager::getRenderTexture(TextureID::Depth);
    const rl::Rectangle srcRect = rl::Rectangle(0, 0, WINDOW_WIDTH_RENDER, -WINDOW_HEIGHT_RENDER);
    const rl::Rectangle dstRect = rl::Rectangle(0, 0, WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME);

    rl::BeginTextureMode(targetDepthTex);
    rl::ClearBackground(Colors::ClearRL);
    rl::DrawTexturePro(allDepthTex, srcRect, dstRect, rl::Vector2{0, 0}, 0.0f, rl::WHITE);
    rl::EndTextureMode();

    // Render to larger occlusion color buf
    // clear the center sector and draw the occlusion stuff visible to the camera
    const rl::Rectangle srcRectGame = rl::Rectangle(0, 0, WINDOW_WIDTH_GAME, -WINDOW_HEIGHT_GAME);
    rl::RenderTexture globalOccl = TextureManager::getRenderTexture(TextureID::OcclusionDepth);
    rl::BeginTextureMode(globalOccl);
    Vector2f centerLoc = gfx::getGISector(8);
    rl::rlSetBlendFactors(RL_ONE, RL_ZERO, RL_FUNC_ADD);
    rl::BeginBlendMode(rl::BLEND_CUSTOM);
    rl::DrawRectangle(centerLoc.x, centerLoc.y, WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME, Colors::ClearRL);

    // draw camera sector
    Vector2f loc = gfx::getGISector(8);
    // rl::DrawRectangle(loc.x, loc.y, WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME, Colors::ClearRL);
    rl::DrawTexturePro(cameraDepthView, srcRectGame, rl::Rectangle{centerLoc.x, centerLoc.y, FWINDOW_WIDTH_GAME, FWINDOW_HEIGHT_GAME},
                       rl::Vector2{0, 0}, 0, rl::WHITE);

    loc = gfx::getGISector(Time.getFrame() % 8);
    rl::DrawRectangle(loc.x, loc.y, WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME, Colors::ClearRL);
    rl::DrawTexturePro(mGIOccluderTexture.getDepth(), srcRectGame, rl::Rectangle{loc.x, loc.y, FWINDOW_WIDTH_GAME, FWINDOW_HEIGHT_GAME},
                       rl::Vector2{0, 0}, 0, rl::WHITE);
    rl::EndBlendMode();
    rl::EndTextureMode();
}

void Renderer::buildDistanceField() const {
    static DistanceField dfShader;
    const rl::RenderTexture occlSrc = TextureManager::getRenderTexture(TextureID::OcclusionDepth);
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
    u32 lastShaderId = 0;
    for (const auto& renderInfo : mRenderQueue.mNormalQueue) {
        if (renderInfo.shader.id != lastShaderId) {
            if (renderInfo.shader.id == 0xffffffff) {
                // -1 maps to default sprite shader
                rl::BeginShaderMode(defaultShader);
            } else {
                rl::BeginShaderMode(renderInfo.shader);
            }
            lastShaderId = renderInfo.shader.id;
        }
        renderInfo.piRender->draw(renderInfo, renderContext);
    }
    rl::EndMode2D();
    rl::EndTextureMode();

    // adjust the camera and virtual ratio to work with a game-resolution camera
    f32 prevVirtualRatio = VIRTUAL_SCREEN_RATIO;
    VIRTUAL_SCREEN_RATIO = 1.0f;  // HACK
    gfx::RenderContext gameRenderContext = renderContext;
    gameRenderContext.camera.offset = rl::Vector2(WINDOW_WIDTH_GAME / 2, WINDOW_HEIGHT_GAME / 2);
    gameRenderContext.isOccludersOnly = true;

    auto renderOccluders = [&](s32 sector, const std::vector<gfx::EntityRenderInfo>& queue) {
        Vector2f offset = gfx::getGISectorOffset(sector);
        gameRenderContext.cameraPosition = renderContext.cameraPosition + offset;
        gameRenderContext.camera.target = (gameRenderContext.cameraPosition * Vector2f(1, -1)).asRL();
        rl::BeginTextureMode(mGIOccluderTexture.tex);
        rl::ClearBackground(Colors::ClearRL);
        rl::BeginMode2D(gameRenderContext.camera);
        for (const auto& renderInfo : queue) {
            if (renderInfo.shader.id != lastShaderId) {
                if (renderInfo.shader.id == 0xffffffff) {
                    // -1 maps to default sprite shader
                    rl::BeginShaderMode(defaultShader);
                } else {
                    rl::BeginShaderMode(renderInfo.shader);
                }
                lastShaderId = renderInfo.shader.id;
            }
            renderInfo.piRender->draw(renderInfo, gameRenderContext);
        }
        rl::EndMode2D();
        rl::EndTextureMode();
    };
    renderOccluders(8, mRenderQueue.mOccluderQueueCamera);

    // COPY CAMERA'S COLOR AND DEPTH BUFFERS FOR LATER
    const rl::Rectangle srcRect = rl::Rectangle(0, 0, WINDOW_WIDTH_GAME, -WINDOW_HEIGHT_GAME);
    const rl::Rectangle dstRect = rl::Rectangle(0, 0, WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME);

    rl::BeginTextureMode(TextureManager::getRenderTexture(TextureID::OcclusionColor));
    rl::ClearBackground(Colors::ClearRL);
    rl::DrawTexturePro(mGIOccluderTexture.tex.texture, srcRect, dstRect, rl::Vector2{0, 0}, 0.0f, rl::WHITE);
    rl::EndTextureMode();

    auto tmp = getTemporaryRT(WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME);
    rl::BeginTextureMode(tmp);
    rl::ClearBackground(Colors::ClearRL);
    rl::DrawTexturePro(mGIOccluderTexture.getDepth(), srcRect, dstRect, rl::Vector2{0, 0}, 0.0f, rl::WHITE);
    rl::EndTextureMode();

    renderOccluders(Time.getFrame() % 8, mRenderQueue.mOccluderQueue);

    VIRTUAL_SCREEN_RATIO = prevVirtualRatio;
    scaleDepthBuffers(renderContext, tmp.texture);
    releaseTemporaryRT(tmp);

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
        return entity1.bottom == entity2.bottom ? entity1.shader.id < entity2.shader.id : entity1.bottom > entity2.bottom;
    } else {
        return entity1.shader.id < entity2.shader.id;
    }
}

void Renderer::buildRenderQueue(Vector2i cameraPosition) {
    // .clear() doesn't affect capacity
    mRenderQueue.clear();

    // Configure camera view box for culling
    const AABB cameraViewBox(cameraPosition, {WINDOW_WIDTH_GAME / 2 + PIXELS_PER_TILE / 2, WINDOW_HEIGHT_GAME / 2 + PIXELS_PER_TILE / 2});
    mRenderQueue.setViewBox(cameraViewBox);
    mRenderQueue.setGIViewBox(AABB(cameraPosition + gfx::getGISectorOffset(Time.getFrame() % 8).as<s32>(), cameraViewBox.getHalf()));

    for (const ecs::RenderSystemPair& renderSystem : World.getRenderSystems()) {
        mRenderQueue.setActiveRenderer(renderSystem.pIRender);
        renderSystem.pIRender->addToQueue(mRenderQueue);
    }

    std::sort(mRenderQueue.mNormalQueue.begin(), mRenderQueue.mNormalQueue.end(), isBelow);
    std::sort(mRenderQueue.mUIQueue.begin(), mRenderQueue.mUIQueue.end(), isBelow);
    std::sort(mRenderQueue.mOccluderQueue.begin(), mRenderQueue.mOccluderQueue.end(), isBelow);
    std::sort(mRenderQueue.mOccluderQueueCamera.begin(), mRenderQueue.mOccluderQueueCamera.end(), isBelow);
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

    mStagingTexture.release();
    mStagingTexture = MultiTexture::create(WINDOW_WIDTH_RENDER, WINDOW_HEIGHT_RENDER, rl::PIXELFORMAT_UNCOMPRESSED_R16G16B16A16);
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
