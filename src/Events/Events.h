#pragma once

#include "Sys/Event.h"

template <typename T>
struct Vector2;

namespace whal {

namespace ecs {
class Entity;
}

struct InputEvent;
struct HitInfo;
struct TileMap;

namespace evt {
class EntityDestroyed : public IEvent<ecs::Entity> {};  // do NOT use for game-logic death
class Collision : public IEvent<ecs::Entity, HitInfo> {};
class Input : public IEvent<InputEvent> {};
class Landing : public IEvent<ecs::Entity> {};
class EnteredLevel : public IEvent<ecs::Entity, TileMap&> {};
class ShaderReload : public IEvent<> {};
class Restart : public IEvent<bool> {};
class Pause : public IEvent<bool> {};                   // game pause
class EnginePause : public IEvent<bool> {};             // special internal pause signal
class WindowResize : public IEvent<Vector2<float>> {};  // arg is resize scalar
}  // namespace evt

}  // namespace whal
