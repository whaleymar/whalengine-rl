#pragma once

namespace whal {

namespace ecs {
class Entity;
}

using Callback = void (*)(ecs::Entity entity);

// TODO convert this into MonoBehavior component
struct CustomUpdate {
    Callback callback;
};

struct OnDeath {
    Callback callback = nullptr;
};

}  // namespace whal
