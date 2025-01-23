#pragma once

#include "Util/Types.h"

namespace whal {

namespace ecs {
class Entity;
}

using Callback = void (*)(ecs::Entity entity);

// TODO convert this into MonoBehavior component
struct CustomUpdate {
    Callback callback = nullptr;
    s32 everyNFrame = 1;
};

struct OnDeath {
    Callback callback = nullptr;
};

}  // namespace whal
