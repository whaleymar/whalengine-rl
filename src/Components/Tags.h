#pragma once

#include "Map/ComponentFactory.h"
namespace whal {

struct Player {};
struct AudioListener {};
struct Particle {};
struct Invisible {};
struct IgnoreTimeModifiers {};
struct IsIdealSpring {};
struct BlocksLight {};
struct MouseCursor {};

struct TagLoader : ISerialize<TagLoader, ComponentFactory> {
    static void loadImpl(ecs::Entity entity, void* data);
};
REGISTER_SERIALIZE(TagLoader)

}  // namespace whal
