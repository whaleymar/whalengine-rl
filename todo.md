# To Do 

## Current Goal: 
- levels!

## levels and game mechanics (each thing should have a level that teaches how to use)
- parachute
- air current (propeller?) -> comes before propeller

## Polish
- Particle emitter component on player hands/feet (?) while RJing
- Particle emitter component on projectile

## Camera / Follow
- pretty awful in general
- different movement types (easein/out stuff)
- be affected by momentum (maybe momentum should be added to velocity.total?)

## Components
- UpdateEntity component - stores a function pointer for a custom update method that is entity-specific and doesn't fit in a specific system

## Gfx
- replace radiance with bloom?
- combine draw and sprite components so depth works for draw
    - tagged union; also add bezier draw component, text draw component


## Physics 
- collider offset
    - replace AABB/Circle center ? don't need them if i have transform

## Map 
- object layers
    - special metadata
        - camera strat (might want to rework)
- SingleEntityLayer
    - basically i want to draw a bunch of tiles and have it (effectively) be one entity that moves together
    - nice when I want more complex geometry or i just want an object to be drawn with tiles
    - implementation plan: create class which layers can use. If they use this SingleEntityLayer class, then create a parent entity which owns all tile entities
    - the layer has component properties
    - all tiles are attached to parent
    - would need some way to say "if collision and collider has parent, try running parent callback"
- background/foreground layers should be written to a texture?
- could try having all entities in a level inactive until an onLevelEntered event happens (and we're entering that specific level)

## Improving compile times:
- would be GREAT to use the <iosfwd> include in Print.h and move the iostream import outside of the header file, but not sure how to do that bc it's templated
- can maybe get <unordered_map> out of InputHandler.h and make it a static var in the source file
- static functions / anonymous namespaces for functions that are only defined/used in source files
- move CallbackMap definition from Physics.h to source file, make it static variable
    - Removing ECS include from System.h makes things slower for some reason

## Misc
- ECS parallelization (low priority)
- Logger queue that runs on another thread
- make component factory functions static 

## Bugs
- cppcheck issues

---------------------------------------------------------------------------------------------------------------------------

# Research & Ideas
things i might want to (re)consider in the future -- ctrl+f for "RESEARCH" 

## System:
- make this an actual framework 
- hot-reloading code (youtube video is bookmarked)

## Map:
- bake tile data into a mesh & use that for lighting
- serializing component structs into Tiled propertytypes would be cool, so I don't have to do so much work to add a new component, but it's probably not feasible bc edge cases
- a metadata tag to say an entity shouldn't active until the player enters its level -- esp useful for something with a lifetime

## Physics:
- a lot of physics stuff (like velocity) is stored as floats even though it could be fixed precision (like nearest tenth of a texel) --> I should use ints for this?
- jumping: instead of applying contant upward velocity, could try reducing gravity while jump button held instead 
    - can also try the high parameter jump that sakurai suggested in his video
- quad tree ray cast

## Other:
- map: support tile rotations / flips? (leaning towards no)
- should use 3rd party lib for Expected cause my impl sucks
- triggers which have some constraint, like X>=50

## ECS:
- the entity.set<T> problem (with IMonitor systems): it doesn't really make sense to handle the problem at the system level, because not all component modifications matter. If anything, could do an event callback for when a component is modified and let systems listen for specific component modifications
