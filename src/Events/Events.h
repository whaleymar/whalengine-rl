#pragma once

#include "Sys/Event.h"

namespace whal {

namespace ecs {
class Entity;
}

struct InputEvent;
struct HitInfo;
struct ActiveLevel;

namespace evt {
class Death : public IEvent<ecs::Entity> {};
class Collision : public IEvent<ecs::Entity, HitInfo> {};
class Input : public IEvent<InputEvent> {};
class Landing : public IEvent<ecs::Entity> {};
class EnteredLevel : public IEvent<ecs::Entity, ActiveLevel&> {};
class ShaderReload : public IEvent<> {};
class Restart : public IEvent<bool> {};
class Pause : public IEvent<bool> {};        // game pause
class EnginePause : public IEvent<bool> {};  // special internal pause signal
class WindowResize : public IEvent<> {};
}  // namespace evt

}  // namespace whal
