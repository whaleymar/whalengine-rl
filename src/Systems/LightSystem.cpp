#include "LightSystem.h"

#include <cmath>
#include <raylib.h>

#include "Components/Draw.h"
#include "Components/Lifetime.h"
#include "Components/Light.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "Events/Events.h"
#include "Gfx/Coordinates.h"
#include "Gfx/Pipeline.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Settings.h"
#include "Systems/TagTrackers.h"
#include "Util/Vector.h"

namespace whal {

const Color COLOR_AMBIENT = Color(0, 0, 0, 255);

void drawLights(Camera2D worldCamera) {
    BeginTextureMode(TextureManager::getRenderTexture(TextureID::Lighting));
    BeginMode2D(worldCamera);
    ClearBackground(COLOR_AMBIENT);

    BeginBlendMode(BLEND_ADDITIVE);
    System::world.getSystem<PointLightSystem>()->update();
    System::world.getSystem<BoxLightSystem>()->update();
    EndMode2D();

    System::world.getSystem<ShadowLightSystem>()->update();
    EndBlendMode();
    EndTextureMode();

    // TODO try scaling the lighting texture up to full resolution, THEN doing blur, and then multiplying? I don't think pixelated lighting looks very
    // good
    static Pipeline lightingPipeline({WINDOW_WIDTH_PIXELS, WINDOW_HEIGHT_PIXELS}, {Shaders::Blur});
    lightingPipeline.process(TextureID::Lighting);
}

// RESEARCH this assumes the entity is rotated about the transform position
static Vector2i getLightOffset(f32 rotationDegrees, s32 lightHeightPixels) {
    return (angleToUnit(-rotationDegrees + 90.0f) * lightHeightPixels).round();
}

void PointLightSystem::onEvent(ShaderReloadEvent) {
    mPositionUniform = GetShaderLocation(ShaderManager::get(Shaders::PointLight), "position");
}

void PointLightSystem::update() {
    auto cameraPos = getCameraPositionPrecise();
    // auto cameraPos = toFloatVec(getCameraPosition());

    Shader shader = ShaderManager::get(Shaders::PointLight);
    ScopedShader shaderScope = ShaderManager::activateScoped(Shaders::PointLight);

    const Texture& randomTexture = TextureManager::instance().getTextureAtlas(TEXNAME_SPRITE).getTexture();
    for (auto [entityid, entity] : getEntitiesMutable()) {
        if (entity.has<Invisible>()) {
            continue;
        }
        PointLight light = entity.get<PointLight>();
        const Vector2i worldPosition =
            entity.get<Transform2D>().position + getLightOffset(entity.get<Transform2D>().rotationDegrees, light.heightTexels * PIXELS_PER_TEXEL);
        const Vector2i screenPosition(worldPosition.x - cameraPos.x, -1 * worldPosition.y + cameraPos.y);
        Color color = Color(light.color.r, light.color.b, light.color.g, light.color.a);

        // for entities with lifetimes, fade out in last moments
        constexpr f32 defaultFadeTime = 0.25f;
        f32 intensity = 1.0;
        s32 radius = light.radiusTexels * PIXELS_PER_TEXEL;
        if (auto fadeoutOpt = entity.tryGet<FadeOut>(); fadeoutOpt) {
            intensity = fadeoutOpt->getIntensity();
        } else if (auto lifetimeOpt = entity.tryGet<Lifetime>(); lifetimeOpt) {
            if (lifetimeOpt->secondsRemaining < defaultFadeTime) {
                intensity = lifetimeOpt->secondsRemaining / defaultFadeTime;
            }
        }

        color.a = std::lerp(COLOR_AMBIENT.a, color.a, intensity);
        radius = std::lerp(radius / 2, radius, intensity);

        Vector2 screenPosV(screenPosition.x, screenPosition.y);
        SetShaderValue(shader, mPositionUniform, &screenPosV, SHADER_UNIFORM_VEC2);

        const Rectangle srcRect(0, 0, randomTexture.width, randomTexture.height);
        const Rectangle dstRect(screenPosition.x - radius, screenPosition.y - radius, radius * 2, radius * 2);
        DrawTexturePro(randomTexture, srcRect, dstRect, Vector2(0, 0), 0, color);
    }
}

void BoxLightSystem::onEvent(ShaderReloadEvent) {
    Shader shader = ShaderManager::get(Shaders::BoxLight);
    mPositionUniform = GetShaderLocation(shader, "lightpos");
    mHalflenUniform = GetShaderLocation(shader, "lighthalflen");
    mRadiusUniform = GetShaderLocation(shader, "lightradius");
}

void BoxLightSystem::update() {
    auto cameraPos = getCameraPositionPrecise();
    // auto cameraPos = toFloatVec(getCameraPosition());

    Shader shader = ShaderManager::get(Shaders::BoxLight);

    Texture randomTexture = TextureManager::getRenderTexture(TextureID::Lighting).texture;
    for (auto [entityid, entity] : getEntitiesMutable()) {
        if (entity.has<Invisible>()) {
            continue;
        }

        // do this every entity so draw calls aren't instanced and the uniform changes
        // should be fine if there aren't a ton of these lights
        ScopedShader shaderScope = ShaderManager::activateScoped(Shaders::BoxLight);

        BoxLight light = entity.get<BoxLight>();
        const Vector2i worldPosition =
            entity.get<Transform2D>().position + getLightOffset(entity.get<Transform2D>().rotationDegrees, light.heightTexels * PIXELS_PER_TEXEL);
        Vector2i screenPosition(worldPosition.x - cameraPos.x, -1 * worldPosition.y + cameraPos.y);
        Color color = Color(light.color.r, light.color.b, light.color.g, light.color.a);

        // for entities with lifetimes, fade out in last moments
        constexpr f32 defaultFadeTime = 0.25f;
        f32 intensity = 1.0;
        s32 radius = light.radiusTexels * PIXELS_PER_TEXEL;
        if (auto fadeoutOpt = entity.tryGet<FadeOut>(); fadeoutOpt) {
            intensity = fadeoutOpt->getIntensity();
        } else if (auto lifetimeOpt = entity.tryGet<Lifetime>(); lifetimeOpt) {
            if (lifetimeOpt->secondsRemaining < defaultFadeTime) {
                intensity = lifetimeOpt->secondsRemaining / defaultFadeTime;
            }
        }

        color.r = std::lerp(COLOR_AMBIENT.r, color.r, intensity);
        color.g = std::lerp(COLOR_AMBIENT.g, color.g, intensity);
        color.b = std::lerp(COLOR_AMBIENT.b, color.b, intensity);
        color.a = std::lerp(COLOR_AMBIENT.a, color.a, intensity);
        radius = std::lerp(radius / 2, radius, intensity);

        Vector2 screenPosV(screenPosition.x, screenPosition.y);
        Vector2 halfLenV(light.halfLenTexels.x * PIXELS_PER_TEXEL, light.halfLenTexels.y * PIXELS_PER_TEXEL);
        f32 fRadius = static_cast<f32>(radius);
        SetShaderValue(shader, mPositionUniform, &screenPosV.x, SHADER_UNIFORM_VEC2);
        SetShaderValue(shader, mHalflenUniform, &halfLenV.x, SHADER_UNIFORM_VEC2);
        SetShaderValue(shader, mRadiusUniform, &fRadius, SHADER_UNIFORM_FLOAT);

        Vector2i lightBounds(radius + light.halfLenTexels.x * PIXELS_PER_TEXEL, radius + light.halfLenTexels.y * PIXELS_PER_TEXEL);
        Rectangle srcRect(0, 0, randomTexture.width, randomTexture.height);
        Rectangle dstRect(screenPosition.x - lightBounds.x, screenPosition.y - lightBounds.y, lightBounds.x * 2, lightBounds.y * 2);

        DrawTexturePro(randomTexture, srcRect, dstRect, Vector2(0, 0), 0, color);
    }
}

void RadianceLightSystem::onEvent(ShaderReloadEvent) {
    mPositionUniform = GetShaderLocation(ShaderManager::get(Shaders::Radiance), "position");
}

void RadianceLightSystem::update(Camera2D worldCamera) {
    auto cameraPos = getCameraPositionPrecise();
    // auto cameraPos = toFloatVec(getCameraPosition());

    Shader shader = ShaderManager::get(Shaders::Radiance);
    ScopedShader shaderScope = ShaderManager::activateScoped(Shaders::Radiance);
    BeginTextureMode(TextureManager::getRenderTexture(TextureID::Radiance));
    BeginMode2D(worldCamera);

    ClearBackground({0, 0, 0, 0});  // don't overwrite background stuff

    const Texture& randomTexture = TextureManager::instance().getTextureAtlas(TEXNAME_SPRITE).getTexture();
    for (auto [entityid, entity] : getEntitiesMutable()) {
        if (entity.has<Invisible>()) {
            continue;
        }

        Radiance light = entity.get<Radiance>();
        const Vector2i worldPosition =
            entity.get<Transform2D>().position + getLightOffset(entity.get<Transform2D>().rotationDegrees, light.heightTexels * PIXELS_PER_TEXEL);
        Vector2i screenPosition(worldPosition.x - cameraPos.x, -1 * worldPosition.y + cameraPos.y);
        Color color = Color(light.color.r, light.color.b, light.color.g, light.color.a);

        // for entities with lifetimes, fade out in last moments
        f32 intensity = 1.0;
        s32 radius = light.radiusTexels * PIXELS_PER_TEXEL;
        if (auto fadeoutOpt = entity.tryGet<FadeOut>(); fadeoutOpt) {
            intensity = fadeoutOpt->getIntensity();
        } else if (auto lifetimeOpt = entity.tryGet<Lifetime>(); lifetimeOpt) {
            if (lifetimeOpt->secondsRemaining < 0.25) {
                intensity = lifetimeOpt->secondsRemaining / 0.25;
            }
        }
        radius = std::lerp(0, radius, intensity);

        Vector2 screenPosV(screenPosition.x, screenPosition.y);
        SetShaderValue(shader, mPositionUniform, &screenPosV, SHADER_UNIFORM_VEC2);

        Rectangle srcRect(0, 0, randomTexture.width, randomTexture.height);
        Rectangle dstRect(screenPosition.x - radius, screenPosition.y - radius, radius * 2, radius * 2);
        DrawTexturePro(randomTexture, srcRect, dstRect, Vector2(0, 0), 0, color);
    }

    EndMode2D();
    EndTextureMode();
}

void ShadowLightSystem::onEvent(ShaderReloadEvent) {
    mLightPosUniform = GetShaderLocation(ShaderManager::get(Shaders::ShadowLight), "lp1");
    mRadiusUniform = GetShaderLocation(ShaderManager::get(Shaders::ShadowLight), "radiusPixels");
}

void ShadowLightSystem::update() {
    // RESEARCH maybe pass angle/spread uniform?

    auto shader = ShaderManager::get(Shaders::ShadowLight);
    for (auto [entityid, entity] : getEntitiesMutable()) {
        ShaderManager::activate(Shaders::ShadowLight);

        const auto light = entity.get<ShadowLight>();
        Vector2i entityPos = entity.get<Transform2D>().position;
        Vector2f screenPos = worldToUVcoords(entityPos.as<f32>() + Vector2f(0, light.heightTexels * PIXELS_PER_TEXEL));
        Vector2 screenPosRL = Vector2(screenPos.x, screenPos.y);
        SetShaderValue(shader, mLightPosUniform, &screenPosRL, SHADER_UNIFORM_VEC2);

        const f32 lightRadiusPixels = light.radiusTexels * PIXELS_PER_TEXEL;
        SetShaderValue(shader, mRadiusUniform, &lightRadiusPixels, SHADER_UNIFORM_FLOAT);
        auto& tex = TextureManager::getRenderTexture(TextureID::Occlusion).texture;
        DrawTextureRec(tex, Rectangle(0, 0, tex.width, -tex.height), Vector2(0, 0), light.color);
        EndShaderMode();
    }
}

}  // namespace whal
