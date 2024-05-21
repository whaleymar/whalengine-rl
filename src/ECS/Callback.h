#pragma once

namespace whal {

namespace ecs {
class Entity;
}

using Callback = void (*)(ecs::Entity entity);

struct OnFrameEnd {
    Callback callback = nullptr;
    bool removeSelf = true;
};

}  // namespace whal
