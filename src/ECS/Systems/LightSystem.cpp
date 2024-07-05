#include "LightSystem.h"

#include <cmath>
#include <raylib.h>

#include "ECS/Draw.h"
#include "ECS/Lifetime.h"
#include "ECS/Light.h"
#include "ECS/Systems/TagTrackers.h"
#include "ECS/Transform.h"
#include "Game.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Settings.h"
#include "raylib/src/raylib.h"

namespace whal {

// TODO should be set in level (maybe make it one of a few options like dark, dim, normal, bright)
// const Color COLOR_AMBIENT = Color(255, 255, 255, 255);
const Color COLOR_AMBIENT = Color(200, 200, 200, 255);
// const Color COLOR_AMBIENT = Color(0, 0, 0, 255);

PointLightSystem::PointLightSystem() {
    mPositionUniform = GetShaderLocation(ShaderManager::get(Shaders::PointLight), "position");
}

void PointLightSystem::update() {
    auto cameraPos = getCameraPositionPrecise();
    // auto cameraPos = toFloatVec(getCameraPosition());

    Shader shader = ShaderManager::get(Shaders::PointLight);
    BeginShaderMode(shader);
    BeginTextureMode(TextureManager::instance().getLightingTexture());
    BeginMode2D(*Game::instance().getWorldCamera());

    ClearBackground(COLOR_AMBIENT);

    const Texture& randomTexture = TextureManager::instance().getTextureAtlas(TEXNAME_SPRITE).getTexture();
    for (auto [entityid, entity] : getEntitiesRef()) {
        PointLight light = entity.get<PointLight>();
        Vector2i worldPosition = entity.get<Transform2D>().position + Vector2i(0, light.heightTexels * PIXELS_PER_TEXEL);
        Vector2i screenPosition(worldPosition.x() - cameraPos.x(), -1 * worldPosition.y() + cameraPos.y());
        Color color = Color(light.color.r, light.color.b, light.color.g, light.color.a);

        // for entities with lifetimes, fade out in last moments
        constexpr f32 defaultFadeTime = 0.25f;
        f32 intensity = 1.0;
        s32 radius = light.radiusTexels * PIXELS_PER_TEXEL;
        if (auto fadeoutOpt = entity.tryGet<FadeOut>(); fadeoutOpt) {
            intensity = (*fadeoutOpt)->getIntensity();
        } else if (auto lifetimeOpt = entity.tryGet<Lifetime>(); lifetimeOpt) {
            if ((*lifetimeOpt)->secondsRemaining < defaultFadeTime) {
                intensity = (*lifetimeOpt)->secondsRemaining / defaultFadeTime;
            }
        }

        color.r = std::lerp(COLOR_AMBIENT.r, color.r, intensity);
        color.g = std::lerp(COLOR_AMBIENT.g, color.g, intensity);
        color.b = std::lerp(COLOR_AMBIENT.b, color.b, intensity);
        color.a = std::lerp(COLOR_AMBIENT.a, color.a, intensity);
        radius = std::lerp(radius / 2, radius, intensity);

        Vector2 screenPosV(screenPosition.x(), screenPosition.y());
        SetShaderValue(shader, mPositionUniform, &screenPosV, SHADER_UNIFORM_VEC2);

        Rectangle srcRect(0, 0, randomTexture.width, randomTexture.height);
        Rectangle dstRect(screenPosition.x() - radius, screenPosition.y() - radius, radius * 2, radius * 2);
        DrawTexturePro(randomTexture, srcRect, dstRect, Vector2(0, 0), 0, color);
    }

    EndMode2D();
    EndTextureMode();
    EndShaderMode();
}

RadianceLightSystem::RadianceLightSystem() {
    mPositionUniform = GetShaderLocation(ShaderManager::get(Shaders::Radiance), "position");
}

void RadianceLightSystem::update() {
    auto cameraPos = getCameraPositionPrecise();
    // auto cameraPos = toFloatVec(getCameraPosition());

    Shader shader = ShaderManager::get(Shaders::Radiance);
    BeginShaderMode(shader);
    BeginTextureMode(TextureManager::instance().getBloomTexture());
    BeginMode2D(*Game::instance().getWorldCamera());

    ClearBackground({0, 0, 0, 0});  // don't overwrite background stuff

    const Texture& randomTexture = TextureManager::instance().getTextureAtlas(TEXNAME_SPRITE).getTexture();
    for (auto [entityid, entity] : getEntitiesRef()) {
        Radiance light = entity.get<Radiance>();
        Vector2i worldPosition = entity.get<Transform2D>().position + Vector2i(0, light.heightTexels * PIXELS_PER_TEXEL);
        Vector2i screenPosition(worldPosition.x() - cameraPos.x(), -1 * worldPosition.y() + cameraPos.y());
        Color color = Color(light.color.r, light.color.b, light.color.g, light.color.a);

        // for entities with lifetimes, fade out in last moments
        f32 intensity = 1.0;
        auto lifetimeOpt = entity.tryGet<Lifetime>();
        s32 radius = light.radiusTexels * PIXELS_PER_TEXEL;
        if (auto fadeoutOpt = entity.tryGet<FadeOut>(); fadeoutOpt) {
            intensity = (*fadeoutOpt)->getIntensity();
        } else if (auto lifetimeOpt = entity.tryGet<Lifetime>(); lifetimeOpt) {
            if ((*lifetimeOpt)->secondsRemaining < 0.25) {
                intensity = (*lifetimeOpt)->secondsRemaining / 0.25;
            }
        }
        radius = std::lerp(0, radius, intensity);

        Vector2 screenPosV(screenPosition.x(), screenPosition.y());
        SetShaderValue(shader, mPositionUniform, &screenPosV, SHADER_UNIFORM_VEC2);

        Rectangle srcRect(0, 0, randomTexture.width, randomTexture.height);
        Rectangle dstRect(screenPosition.x() - radius, screenPosition.y() - radius, radius * 2, radius * 2);
        DrawTexturePro(randomTexture, srcRect, dstRect, Vector2(0, 0), 0, color);
    }

    EndMode2D();
    EndTextureMode();
    EndShaderMode();
}

}  // namespace whal
