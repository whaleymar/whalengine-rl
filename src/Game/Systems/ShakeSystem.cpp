#include "ShakeSystem.h"
#include "Components/Tween.h"
#include "Game/Components/Shake.h"
#include "Sys/System.h"

using namespace whal;

void ShakeSystem::onAdd(ecs::Entity entity) {
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
        Vector2i offset = Vector2i::zero;
        if (shake.duration > 0.0f) {
            offset = (Vector2f(shake.strength, shake.strength) * Vector2f(System::rng.uniform(), System::rng.uniform())).round();
        }
        entity.get<Draw>().offset = offset;
        shake.duration -= dt;
    }
}
