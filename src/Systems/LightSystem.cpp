#include "LightSystem.h"

#include <cmath>
#include <raylib.h>
#include <rlgl.h>

#include "Components/Light.h"
#include "Components/Tags.h"
#include "Components/Transform.h"

#include "Events/Events.h"
#include "Gfx/Color.h"
#include "Gfx/Coordinates.h"
#include "Gfx/RaylibUtil.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Settings.h"
#include "Sys/System.h"
#include "Systems/TagSystems.h"

#include "Util/CameraUtil.h"
#include "Util/Easing.h"
#include "Util/Vector.h"

namespace whal {

const Color COLOR_AMBIENT = Colors::Black;

void PointLightSystem::onEvent(evt::ShaderReload) {
    mPositionUniform = GetShaderLocation(ShaderManager::get(Shaders::PointLight), "position");
    // mLightDepthUniform = GetShaderLocation(ShaderManager::get(Shaders::ShadowLight), "lightDepth");
    // mOcclusionDepthUniform = GetShaderLocation(ShaderManager::get(Shaders::ShadowLight), "occlusionDepthTex");
}

void PointLightSystem::draw(const gfx::RenderContext& ctx) const {
    auto cameraPos = getCameraPositionPrecise();

    rl::Shader shader = ShaderManager::get(Shaders::PointLight);
    ScopedShader shaderScope = ShaderManager::activateScoped(Shaders::PointLight);

    // const auto depthTex = TextureManager::getRenderTexture(TextureID::OcclusionDepth).texture;
    const auto colorTex = TextureManager::getRenderTexture(TextureID::OcclusionColor).texture;
    for (auto [entityid, entity] : getEntitiesMutable()) {
        if (entity.has<Invisible>()) {
            continue;
        }
        PointLight light = entity.get<PointLight>();
        const auto trans = entity.get<Transform>();
        const Vector2i worldPosition = trans.apply(Vector2i(0, light.heightOffset));
        const Vector2i screenPosition =
            Vector2i(worldPosition.x - cameraPos.x, -1 * worldPosition.y + cameraPos.y) + Vector2i(WINDOW_WIDTH_GAME / 2, WINDOW_HEIGHT_GAME / 2);
        Color color = light.color;

        // RESEARCH may want to put this as a param in the component
        constexpr f32 intensity = 1.0;
        s32 radius = light.radius;

        color.a = std::lerp(COLOR_AMBIENT.a, color.a, intensity);
        radius = ease(radius / 2, radius, intensity, Ease::InQuad);

        rl::Vector2 screenPosV(screenPosition.x, screenPosition.y);
        rl::SetShaderValue(shader, mPositionUniform, &screenPosV, rl::SHADER_UNIFORM_VEC2);

        // const f32 lightDepth = depthToFloat(trans.depth);
        // SetShaderValueTexture(shader, mOcclusionDepthUniform, depthTex);
        // SetShaderValue(shader, mLightDepthUniform, &lightDepth, SHADER_UNIFORM_FLOAT);

        const rl::Rectangle srcRect(0, 0, colorTex.width, colorTex.height);
        const rl::Rectangle dstRect(screenPosition.x - radius, screenPosition.y - radius, radius * 2, radius * 2);
        gfx::DrawSpriteHDR(colorTex, srcRect, dstRect, rl::Vector2(0, 0), 0, color);
    }
}

void BoxLightSystem::onEvent(evt::ShaderReload) {
    rl::Shader shader = ShaderManager::get(Shaders::BoxLight);
    mPositionUniform = GetShaderLocation(shader, "lightpos");
    mHalflenUniform = GetShaderLocation(shader, "lighthalflen");
    mRadiusUniform = GetShaderLocation(shader, "lightradius");
    // mLightDepthUniform = GetShaderLocation(shader, "lightDepth");
    // mOcclusionDepthUniform = GetShaderLocation(shader, "occlusionDepthTex");
}

void BoxLightSystem::draw(const gfx::RenderContext& ctx) const {
    auto cameraPos = getCameraPositionPrecise();
    rl::Shader shader = ShaderManager::get(Shaders::BoxLight);

    const auto randomTexture = Graphics.getTemporaryRT(WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME);
    for (auto [entityid, entity] : getEntitiesMutable()) {
        if (entity.has<Invisible>()) {
            continue;
        }

        // do this every entity so draw calls aren't instanced and the uniform changes
        // should be fine if there aren't a ton of these lights
        ScopedShader shaderScope = ShaderManager::activateScoped(Shaders::BoxLight);

        BoxLight light = entity.get<BoxLight>();
        const auto trans = entity.get<Transform>();
        const Vector2i worldPosition = trans.apply(Vector2i(0, light.heightOffset));
        Vector2i screenPosition =
            Vector2i(worldPosition.x - cameraPos.x, -1 * worldPosition.y + cameraPos.y) + Vector2i(WINDOW_WIDTH_GAME / 2, WINDOW_HEIGHT_GAME / 2);
        Color color = light.color;

        // RESEARCH may want to put this as a param in the component
        constexpr f32 intensity = 1.0;
        s32 radius = light.radius;

        // apply fading
        color = Color::lerp(COLOR_AMBIENT, color, intensity);

        // light falls off quadratically
        radius = ease(radius / 2, radius, intensity, Ease::InQuad);

        rl::Vector2 screenPosV(screenPosition.x, screenPosition.y);
        rl::Vector2 halfLenV(light.halfLen.x, light.halfLen.y);
        f32 fRadius = static_cast<f32>(radius);
        rl::SetShaderValue(shader, mPositionUniform, &screenPosV.x, rl::SHADER_UNIFORM_VEC2);
        rl::SetShaderValue(shader, mHalflenUniform, &halfLenV.x, rl::SHADER_UNIFORM_VEC2);
        rl::SetShaderValue(shader, mRadiusUniform, &fRadius, rl::SHADER_UNIFORM_FLOAT);

        // const f32 lightDepth = depthToFloat(trans.depth);
        // SetShaderValue(shader, mLightDepthUniform, &lightDepth, SHADER_UNIFORM_FLOAT);
        // SetShaderValueTexture(shader, mOcclusionDepthUniform, depthTex);

        const Vector2i lightBounds(radius + light.halfLen.x, radius + light.halfLen.y);
        const Vector2i destPosition = screenPosition - lightBounds;
        const Vector2i destSize = lightBounds * 2;

        const rl::Rectangle srcRect(0, 0, randomTexture.texture.width, randomTexture.texture.height);
        const rl::Rectangle dstRect(destPosition.x, destPosition.y, destSize.x, destSize.y);

        gfx::DrawSpriteHDR(randomTexture.texture, srcRect, dstRect, rl::Vector2(0, 0), 0, color);
    }
    Graphics.releaseTemporaryRT(randomTexture);
}

void ShadowLightSystem::onEvent(evt::ShaderReload) {
    mLightPosUniform = rl::GetShaderLocation(ShaderManager::get(Shaders::ShadowLight), "lp1");
    mRadiusUniform = rl::GetShaderLocation(ShaderManager::get(Shaders::ShadowLight), "radiusPixels");
    mLightDepthUniform = rl::GetShaderLocation(ShaderManager::get(Shaders::ShadowLight), "lightDepth");
    mDepthBufUniform = rl::GetShaderLocation(ShaderManager::get(Shaders::ShadowLight), "depthBuf");
    mOcclDepthBufUniform = rl::GetShaderLocation(ShaderManager::get(Shaders::ShadowLight), "occlDepthBuf");
}

void ShadowLightSystem::draw(const gfx::RenderContext& ctx) const {
    // RESEARCH maybe pass angle/spread uniform?
    // RESEARCH instead of binding new uniforms for every draw call, it would make more sense to pass an array of uniforms to the shader once

    const auto shader = ShaderManager::get(Shaders::ShadowLight);
    const auto depthTex = TextureManager::getRenderTexture(TextureID::AllDepth).texture;
    const auto occlDepthTex = TextureManager::getRenderTexture(TextureID::OcclusionDepth).texture;
    const auto colorTex = TextureManager::getRenderTexture(TextureID::OcclusionColor);

    // must match what's in spritefrag.glsl
    const f32 depthScalar = 20.0f;
    for (auto [entityid, entity] : getEntitiesMutable()) {
        ShaderManager::activate(Shaders::ShadowLight);

        const auto light = entity.get<ShadowLight>();
        const auto trans = entity.get<Transform>();

        const Vector2i entityPos = trans.position;
        const Vector2f screenPos = worldToUVcoords(entityPos.as<f32>() + Vector2f(0, light.heightOffset));
        const rl::Vector2 screenPosRL = rl::Vector2(screenPos.x, screenPos.y);
        const f32 lightRadiusPixels = light.radius;
        const f32 lightDepth = static_cast<f32>(trans.depth) / 255.0f * depthScalar;

        // Set shader values
        rl::SetShaderValue(shader, mLightPosUniform, &screenPosRL, rl::SHADER_UNIFORM_VEC2);
        rl::SetShaderValue(shader, mRadiusUniform, &lightRadiusPixels, rl::SHADER_UNIFORM_FLOAT);
        rl::SetShaderValue(shader, mLightDepthUniform, &lightDepth, rl::SHADER_UNIFORM_FLOAT);
        rl::SetShaderValueTexture(shader, mDepthBufUniform, depthTex);
        rl::SetShaderValueTexture(shader, mOcclDepthBufUniform, occlDepthTex);

        gfx::DrawRenderTextureHDR(colorTex, light.color);
        rl::EndShaderMode();
    }
}

}  // namespace whal
