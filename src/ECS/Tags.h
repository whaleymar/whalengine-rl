#pragma once

// RESEARCH this is stupid
// Tags should be an enum
// and there should just be one "Tags" component
// and all entities should probably have it
// and I just do entity.get<Tags>().has(Tag)
// Or on the ECS side I can just say tags are ints
// and have a entity.hasTag(int tag) ( and hasTags(... idk what i'd pass, maybe a bitset))
// -- i could even do a bitset if I'm feeling craz

// I could even use this for PHYSICS stuff
// actor, solid, semisolid tags
// and consolidate the messy ass collider components/inheritance mess

// HOWEVER, i do want quick access to things like the player and camera, which won't happen if i have to iterate through all entities and check their
// tags so those things being their own tag components still makes sense BUT things like collider type could still work as a single component

namespace whal {

// enum Tag {
//     Player = 1,
//     Camera = 1 << 1,
//     Tile = 1 << 2,  // SafeGround?
// };

struct Player {};

struct Camera {};

}  // namespace whal
