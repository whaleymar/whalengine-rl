#include "LightSystem.h"

#include <cmath>
#include <raylib.h>

#include "ECS/Lifetime.h"
#include "ECS/Light.h"
#include "ECS/Systems/TagTrackers.h"
#include "ECS/Transform.h"
#include "Game.h"
#include "Gfx/Texture.h"
#include "raylib/src/raylib.h"

namespace whal {

// TODO should be set in level (maybe make it one of a few options like dark, dim, normal, bright)
// const Color COLOR_AMBIENT = Color(200, 200, 200, 255);
const Color COLOR_AMBIENT = Color(0, 0, 0, 255);

void PointLightSystem::update() {
    auto cameraPos = getCameraPositionPrecise();
    // auto cameraPos = toFloatVec(getCameraPosition());

    BeginShaderMode(*mShaderPtr);
    BeginTextureMode(TextureManager::instance().getLightingTexture());
    BeginMode2D(*Game::instance().getWorldCamera());

    ClearBackground(COLOR_AMBIENT);

    const Texture& randomTexture = TextureManager::instance().getTextureAtlas(TEXNAME_SPRITE).getTexture();
    for (auto [entityid, entity] : getEntitiesRef()) {
        PointLight light = entity.get<PointLight>();
        Vector2i worldPosition = entity.get<Transform2D>().position + Vector2i(0, light.height);
        Vector2i screenPosition(worldPosition.x() - cameraPos.x(), -1 * worldPosition.y() + cameraPos.y());
        Color color = Color(light.color.r, light.color.b, light.color.g, light.color.a);

        // for entities with lifetimes, fade out in last moments
        f32 intensity = 1.0;
        auto lifetimeOpt = entity.tryGet<Lifetime>();
        s32 radius = light.radius;
        if (lifetimeOpt) {
            if ((*lifetimeOpt)->secondsRemaining < 0.25) {
                intensity = (*lifetimeOpt)->secondsRemaining / 0.25;
                color.r = std::lerp(COLOR_AMBIENT.r, color.r, intensity);
                color.g = std::lerp(COLOR_AMBIENT.g, color.g, intensity);
                color.b = std::lerp(COLOR_AMBIENT.b, color.b, intensity);
                color.a = std::lerp(COLOR_AMBIENT.a, color.a, intensity);
                radius = std::lerp(0, radius, intensity);
            }
        }

        Vector2 screenPosV(screenPosition.x(), screenPosition.y());
        SetShaderValue(*mShaderPtr, mPositionUniform, &screenPosV, SHADER_UNIFORM_VEC2);

        Rectangle srcRect(0, 0, randomTexture.width, randomTexture.height);
        Rectangle dstRect(screenPosition.x() - radius, screenPosition.y() - radius, radius * 2, radius * 2);
        DrawTexturePro(randomTexture, srcRect, dstRect, Vector2(0, 0), 0, color);
    }

    EndMode2D();
    EndTextureMode();
    EndShaderMode();
}

void RadianceLightSystem::update() {
    auto cameraPos = getCameraPositionPrecise();
    // auto cameraPos = toFloatVec(getCameraPosition());

    BeginShaderMode(*mShaderPtr);
    BeginMode2D(*Game::instance().getWorldCamera());

    const Texture& randomTexture = TextureManager::instance().getTextureAtlas(TEXNAME_SPRITE).getTexture();
    for (auto [entityid, entity] : getEntitiesRef()) {
        Radiance light = entity.get<Radiance>();
        Vector2i worldPosition = entity.get<Transform2D>().position + Vector2i(0, light.height);
        Vector2i screenPosition(worldPosition.x() - cameraPos.x(), -1 * worldPosition.y() + cameraPos.y());
        Color color = Color(light.color.r, light.color.b, light.color.g, light.color.a);

        // for entities with lifetimes, fade out in last moments
        f32 intensity = 1.0;
        auto lifetimeOpt = entity.tryGet<Lifetime>();
        s32 radius = light.radius;
        if (lifetimeOpt) {
            if ((*lifetimeOpt)->secondsRemaining < 0.25) {
                intensity = (*lifetimeOpt)->secondsRemaining / 0.25;
                radius = std::lerp(0, radius, intensity);
            }
        }

        Vector2 screenPosV(screenPosition.x(), screenPosition.y());
        SetShaderValue(*mShaderPtr, mPositionUniform, &screenPosV, SHADER_UNIFORM_VEC2);

        Rectangle srcRect(0, 0, randomTexture.width, randomTexture.height);
        Rectangle dstRect(screenPosition.x() - radius, screenPosition.y() - radius, radius * 2, radius * 2);
        DrawTexturePro(randomTexture, srcRect, dstRect, Vector2(0, 0), 0, color);
    }

    EndMode2D();
    EndShaderMode();
}

}  // namespace whal
