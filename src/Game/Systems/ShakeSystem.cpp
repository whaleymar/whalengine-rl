#include "ShakeSystem.h"
#include "Components/Transform.h"
#include "Components/Tween.h"
#include "Game/Components/Shake.h"
#include "Sys/System.h"

using namespace whal;

void ShakeSystem::onAdd(ecs::Entity entity) {
    // tween shake strength from the current value to zero, and remove the component when complete
    auto shake = entity.get<Shake>();
    TweenManager::add(TweenFloat(0.0f, shake.duration, [](ecs::Entity self) -> f32& { return self.get<Shake>().strength; })
                          .setTransition(shake.easeFunc)
                          .setOnEnd([](ecs::Entity self, const TweenFloat&) { self.remove<Shake>(); }),
                      entity);
}

void ShakeSystem::update() {
    const f32 dt = System::dt();
    for (auto [entityid, entity] : getEntitiesMutable()) {
        auto& shake = entity.get<Shake>();
        Vector2f offsetF = Vector2f::zero;
        Vector2i offset = Vector2i::zero;
        if (shake.duration > 0.0f) {
            offsetF = Vector2f(shake.strength, shake.strength) * Vector2f(System::rng.range(-1.0f, 1.0f), System::rng.range(-1.0f, 1.0f));
            offset = offsetF.round();
        }

        // update transform (and precise position if relevant)
        auto& trans = entity.get<Transform2D>();
        Vector2i transPositionNoOffset = trans.position - shake.previousOffset.round();
        trans.position = transPositionNoOffset + offset;

        if (entity.has<PrecisePosition>()) {
            auto& precisePosition = entity.get<PrecisePosition>();
            Vector2f precisePositionNoOffset = precisePosition.position - shake.previousOffset;
            precisePosition.position = precisePositionNoOffset + offsetF;
        }

        shake.previousOffset = offsetF;
        shake.duration -= dt;
    }
}
