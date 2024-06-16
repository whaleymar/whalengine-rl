#pragma once

#include "Systems/Event.h"
#include "Util/Vector.h"

namespace whal::ecs {
class Entity;
}

class ShootEvent : public whal::IEvent<Vector2i> {};

namespace GameEvent {

inline const ShootEvent SHOOT_EVENT;

}  // namespace GameEvent
