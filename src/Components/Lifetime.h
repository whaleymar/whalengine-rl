#pragma once

#include "Components/Draw.h"
#include "Util/Types.h"

namespace whal {

namespace ecs {
class Entity;
}

struct Lifetime {
    using Callback = void (*)(ecs::Entity entity);
    f32 secondsRemaining;
    Callback onDeath = nullptr;
};

struct DieWhenSpeedBelow {
    f32 minSpeed;
    ColorLerp colorFade;
};

}  // namespace whal
