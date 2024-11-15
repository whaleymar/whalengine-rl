#include "NavigationSystem.h"
#include "Sys/System.h"

#ifndef NDEBUG
#include "Components/Transform.h"
#include "Events/Events.h"
#include "IGame.h"
#include "Map/Level.h"
#include "Map/Path.h"
#include "Sys/InputHandler.h"
#include "Systems/Graphics/Common.h"
#include "Systems/TagSystems.h"

namespace whal {

void NavigationSystem::onEvent(evt::ButtonPress, InputType input) {
    if (input != InputType::M1) {
        return;
    }
    auto pSys = World.getSystem<PlayerSystem>();
    if (pSys->getEntitiesMutable().empty()) {
        return;
    }
    auto start = pSys->first().get<Transform2D>().position;
    auto end = Input.getMouseWorld();
    auto lvl = System::getGame().getScene().getLevelAt(start);
    if (!lvl) {
        return;
    }

    auto active = System::getGame().getScene().getLoadedLevel(*lvl);
    if (active.isExpected()) {
        Path path = findPath(start, end, *active.value());
        pSys->first().add(path);
    }
}

void NavigationSystem::draw(ecs::Entity entity, const gfx::RenderContext ctx) const {
    // TEMP testing tile coordinate functions
    const auto trans = entity.get<Transform2D>();
    const auto path = entity.get<Path>();

    Vector2f tileCoordStart = (((path.start + Vector2i(0, -8)) / 8) * 8).as<f32>();

    const Vector2f frameSize(8, 8);
    for (auto step : path.tiles) {
        PreciseTransform2D tileCoord = PreciseTransform2D(tileCoordStart + Vector2f(8, 8) * step.as<f32>());
        tileCoordStart = tileCoord.position;
        const gfx::RaylibDrawParams params = gfx::getDrawParams(tileCoord, frameSize, ctx.cameraPosition);
        DrawRectanglePro(params.rect, params.origin, 0.0f, Color{100, 100, 255, 100});
    }
}

void NavigationSystem::addToQueue(std::vector<gfx::EntityRenderInfo>& queue) const {
    if (!Input.isOn(InputType::DEBUG)) {
        return;
    }
    for (auto [entityid, entity] : getEntitiesMutable()) {
        const auto trans = entity.get<Transform2D>();
        const auto path = entity.get<Path>();

        queue.emplace_back(gfx::EntityRenderInfo{
            .boundingBox = AABB::fromPoints(path.start, path.target),
            .depth = Depth::Debug,
            .entity = entity,
            .piRender = this,
        });
    }
}

}  // namespace whal
#endif
