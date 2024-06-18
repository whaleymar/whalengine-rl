#include "LightSystem.h"

#include <cmath>
#include <raylib.h>

#include "ECS/Lifetime.h"
#include "ECS/Light.h"
#include "ECS/Systems/TagTrackers.h"
#include "ECS/Transform.h"
#include "Game.h"
#include "Gfx/Texture.h"

namespace whal {

const Color COLOR_AMBIENT = Color(150, 150, 150, 255);  // TODO should be set in level

void PointLightSystem::update() {
    auto cameraPos = getCameraPositionPrecise();
    // auto cameraPos = toFloatVec(getCameraPosition());
    BeginTextureMode(TextureManager::instance().getLightingTexture());
    BeginMode2D(*Game::instance().getWorldCamera());

    ClearBackground(COLOR_AMBIENT);

    for (auto [entityid, entity] : getEntitiesRef()) {
        PointLight light = entity.get<PointLight>();
        Vector2i worldPosition = entity.get<Transform2D>().position + Vector2i(0, light.height);
        Vector2i screenPosition(worldPosition.x() - cameraPos.x(), -1 * worldPosition.y() + cameraPos.y());
        Color color = Color(light.color.red, light.color.blue, light.color.green, 255);

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

        DrawCircleGradient(screenPosition.x(), screenPosition.y(), light.radius, color, COLOR_AMBIENT);
    }

    EndMode2D();
    EndTextureMode();
}

}  // namespace whal
