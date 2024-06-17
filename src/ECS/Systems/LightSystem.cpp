#include "LightSystem.h"

#include <raylib.h>

#include "ECS/Lifetime.h"
#include "ECS/Light.h"
#include "ECS/Systems/TagTrackers.h"
#include "ECS/Transform.h"
#include "Game.h"
#include "Gfx/Texture.h"

namespace whal {

const Color COLOR_AMBIENT = Color(50, 50, 50, 255);  // TODO should be set in level

void PointLightSystem::update() {
    auto cameraPos = getCameraPosition();
    BeginTextureMode(TextureManager::instance().getLightingTexture());
    BeginMode2D(*Game::instance().getWorldCamera());

    ClearBackground(COLOR_AMBIENT);

    for (auto [entityid, entity] : getEntitiesRef()) {
        PointLight light = entity.get<PointLight>();
        Vector2i worldPosition = entity.get<Transform2D>().position + Vector2i(0, light.height);
        Vector2i screenPosition = {worldPosition.x() - cameraPos.x(), -1 * worldPosition.y() + cameraPos.y()};

        // for entities with lifetimes, fade out in last second
        f32 intensity = 1.0;
        auto lifetimeOpt = entity.tryGet<Lifetime>();
        if (lifetimeOpt) {
            if ((*lifetimeOpt)->secondsRemaining < 1) {
                intensity = (*lifetimeOpt)->secondsRemaining / 1;
            }
        }

        s32 alpha = static_cast<s32>(255.0f * intensity);
        Color color = Color(light.color.red, light.color.blue, light.color.green, alpha);
        Color ambient = COLOR_AMBIENT;
        ambient.a = alpha;
        DrawCircleGradient(screenPosition.x(), screenPosition.y(), light.radius, color, ambient);
    }

    EndMode2D();
    EndTextureMode();
}

}  // namespace whal
