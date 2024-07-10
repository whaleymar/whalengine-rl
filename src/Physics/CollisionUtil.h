#pragma once

#include "Util/Vector.h"

namespace whal {

namespace ecs {
class Entity;
}

class Collider;
class AABB;

using CollisionCallback = void (*)(ecs::Entity callbackEntity, ecs::Entity other, Vector2i hitNormal);

enum class CollisionDir : u8 { ALL, LEFT, RIGHT, DOWN, UP };

// returns true if a collision CAN happen given the move normal & collision direction
bool checkDirectionalCollision(const AABB& actor, const AABB& solid, Vector2i moveNormal, CollisionDir collisionDir);

}  // namespace whal
