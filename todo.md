# To Do 

## Current Goal: 
- camera rework

## Camera / Follow
- pretty awful in general
- different movement types (easein/out stuff)
- be affected by momentum (maybe momentum should be added to velocity.total?)

## Entity Prefabs
- death plane

## Components
- UpdateEntity component - stores a function pointer for a custom update method that is entity-specific and doesn't fit in a specific system

## Gfx
- replace radiance with bloom?

## Map 
- object layers
    - special metadata
        - camera strat (might want to rework)

## Improving compile times:
- would be GREAT to use the <iosfwd> include in Print.h and move the iostream import outside of the header file, but not sure how to do that bc it's templated
- can maybe get <unordered_map> out of InputHandler.h and make it a static var in the source file
- static functions / anonymous namespaces for functions that are only defined/used in source files
- move CallbackMap definition from Physics.h to source file, make it static variable
    - Removing ECS include from System.h makes things slower for some reason

## Misc
- ECS parallelization (low priority)
- Logger queue that runs on another thread

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
