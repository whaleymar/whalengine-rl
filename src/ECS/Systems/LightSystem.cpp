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

const Color COLOR_AMBIENT =
    Color(200, 200, 200, 255);  // TODO should be set in level (maybe make it one of a few options like dark, dim, normal, bright)

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
        Color color = Color(light.color.red, light.color.blue, light.color.green, light.color.alpha);

        // for entities with lifetimes, fade out in last moments
        f32 intensity = 1.0;
        auto lifetimeOpt = entity.tryGet<Lifetime>();
        if (lifetimeOpt) {
            if ((*lifetimeOpt)->secondsRemaining < 0.25) {
                intensity = (*lifetimeOpt)->secondsRemaining / 0.25;
                color.r = std::lerp(COLOR_AMBIENT.r, color.r, intensity);
                color.g = std::lerp(COLOR_AMBIENT.g, color.g, intensity);
                color.b = std::lerp(COLOR_AMBIENT.b, color.b, intensity);
                color.a = std::lerp(COLOR_AMBIENT.a, color.a, intensity);
            }
        }

        Vector2 screenPosV(screenPosition.x(), screenPosition.y());
        SetShaderValue(*mShaderPtr, mPositionUniform, &screenPosV, SHADER_UNIFORM_VEC2);

        Rectangle srcRect(0, 0, randomTexture.width, randomTexture.height);
        Rectangle dstRect(screenPosition.x() - light.radius, screenPosition.y() - light.radius, light.radius * 2, light.radius * 2);
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
        Color color = Color(light.color.red, light.color.blue, light.color.green, light.color.alpha);

        Vector2 screenPosV(screenPosition.x(), screenPosition.y());
        SetShaderValue(*mShaderPtr, mPositionUniform, &screenPosV, SHADER_UNIFORM_VEC2);

        Rectangle srcRect(0, 0, randomTexture.width, randomTexture.height);
        Rectangle dstRect(screenPosition.x() - light.radius, screenPosition.y() - light.radius, light.radius * 2, light.radius * 2);
        DrawTexturePro(randomTexture, srcRect, dstRect, Vector2(0, 0), 0, color);
        // DrawTexturePro(randomTexture, srcRect, dstRect, Vector2(0, 0), 0, WHITE);
    }

    EndMode2D();
    EndShaderMode();
}

}  // namespace whal
