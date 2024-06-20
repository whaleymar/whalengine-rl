#pragma once

#include "Systems/Event.h"

namespace whal {

namespace ecs {
class Entity;
}
enum class InputType : u64;
struct HitInfo;
struct ActiveLevel;

class DeathEvent : public IEvent<ecs::Entity> {};
class CollisionEvent : public IEvent<ecs::Entity, HitInfo> {};
class ButtonPressEvent : public IEvent<InputType> {};
class ButtonPressOrReleaseEvent : public IEvent<InputType, bool> {};
class LandingEvent : public IEvent<ecs::Entity> {};
class EnteredLevelEvent : public IEvent<ecs::Entity, ActiveLevel&> {};

}  // namespace whal
