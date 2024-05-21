#pragma once

#include "Systems/Event.h"
#include "Util/Vector.h"

namespace whal {

namespace ecs {
class Entity;
}

class DeathEvent : public IEvent<ecs::Entity> {};
class CollisionEvent : public IEvent<ecs::Entity, ecs::Entity> {};
class ShootEvent : public IEvent<Vector2i> {};

namespace Event {

inline const DeathEvent DEATH_EVENT;
inline const CollisionEvent COLLISION_EVENT;
inline const ShootEvent SHOOT_EVENT;

}  // namespace Event

}  // namespace whal
