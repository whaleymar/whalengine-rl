#pragma once

#include "Systems/Event.h"

namespace whal {

namespace ecs {
class Entity;
}
enum class InputType : u64;
struct HitInfo;

class DeathEvent : public IEvent<ecs::Entity> {};
class CollisionEvent : public IEvent<ecs::Entity, HitInfo> {};
class ButtonPressEvent : public IEvent<InputType> {};
class ButtonPressOrReleaseEvent : public IEvent<InputType, bool> {};
class LandingEvent : public IEvent<ecs::Entity> {};

namespace Event {

inline const DeathEvent DEATH_EVENT;
inline const CollisionEvent COLLISION_EVENT;
inline const ButtonPressEvent BUTTON_EVENT_PRESS;
inline const ButtonPressOrReleaseEvent BUTTON_EVENT_PRESSRELEASE;
inline const LandingEvent LANDING_EVENT;

}  // namespace Event

}  // namespace whal
