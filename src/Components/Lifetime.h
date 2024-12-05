#pragma once

#include "Map/ComponentFactory.h"
#include "Map/Tiled.h"
#include "Util/Types.h"

namespace whal {

namespace ecs {
class Entity;
}

struct Lifetime : ISerialize<Lifetime, ComponentFactory> {
    using Callback = void (*)(ecs::Entity entity);
    f32 secondsRemaining;
    Callback onDeath = nullptr;

    static void loadImpl(ecs::Entity entity, void* data) {
        const LoadContext& ctx = *static_cast<LoadContext*>(data);
        Lifetime lifetime = entity.has<Lifetime>() ? entity.get<Lifetime>() : Lifetime{.secondsRemaining = 0};
        tryReadFloat(ctx.values, "seconds", &lifetime.secondsRemaining);
        entity.add(lifetime);
    }
};

}  // namespace whal
