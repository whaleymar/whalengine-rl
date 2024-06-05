#pragma once

#include "Systems/Event.h"
#include "Util/Vector.h"

namespace whal {

namespace ecs {
class Entity;
}
enum class InputType : u64;

class DeathEvent : public IEvent<ecs::Entity> {};
class CollisionEvent : public IEvent<ecs::Entity, ecs::Entity> {};
class ShootEvent : public IEvent<Vector2i> {};
class ButtonEvent : public IEvent<InputType> {};
class LandingEvent : public IEvent<ecs::Entity> {};

namespace Event {

inline const DeathEvent DEATH_EVENT;
inline const CollisionEvent COLLISION_EVENT;
inline const ShootEvent SHOOT_EVENT;
inline const ButtonEvent BUTTON_EVENT;
inline const LandingEvent LANDING_EVENT;

}  // namespace Event

}  // namespace whal
