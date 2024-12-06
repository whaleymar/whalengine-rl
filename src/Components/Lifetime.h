#pragma once

#include "Map/ComponentFactory.h"
#include "Util/Types.h"

namespace whal {

namespace ecs {
class Entity;
}

struct Lifetime : ISerialize<Lifetime, ComponentFactory> {
    using Callback = void (*)(ecs::Entity entity);
    f32 secondsRemaining;
    Callback onDeath = nullptr;
};

}  // namespace whal
