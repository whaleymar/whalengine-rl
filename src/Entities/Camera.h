#pragma once

namespace whal {

namespace ecs {
class Entity;
}

struct Transform;

ecs::Entity createCamera(Transform transform);

}  // namespace whal
