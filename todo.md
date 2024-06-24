# To Do 

## Current Goal: 

## Camera / Follow
- pretty awful in general
- different movement types (easein/out stuff)
- be affected by momentum (maybe momentum should be added to velocity.total?)

## Entity Prefabs
- particle
    - draw(?), position, velocity, lifetime
- death plane
- read prefab component values from YAML
    - combined w/ hot reloading, would make fine tuning prototypes easier

## Physics
- chunk loading/unloading (quad tree?)

## Map 
- object layers
    - component factory functions (mostly done)
    - special metadata
        - camera strat (might want to rework)
    - Templates - use for prefabs?

## Graphics
- outline shader

## Improving compile times:
- would be GREAT to use the <iosfwd> include in Print.h and move the iostream import outside of the header file, but not sure how to do that bc it's templated
- can maybe get <unordered_map> out of InputHandler.h and make it a static var in the source file
- static functions / anonymous namespaces for functions that are only defined/used in source files
- move CallbackMap definition from Physics.h to source file, make it static variable
    - Removing ECS include from System.h makes things slower for some reason

## Misc
- ECS lib tasks
- Logger queue that runs on another thread

## Bugs
- isNearZero not working
- cppcheck issues

---------------------------------------------------------------------------------------------------------------------------

# Research & Ideas
things i might want to (re)consider in the future -- ctrl+f for "RESEARCH" 

## System:
- make this an actual framework 
- hot-reloading code (youtube video is bookmarked)

## Map:
- bake tile data into a mesh & use that for lighting
- should also give these tiles a Tile component that I can use for something like updating last safe point player was standing on

## Physics:
- a lot of physics stuff (like velocity) is stored as floats even though it could be fixed precision (like nearest tenth of a texel) --> I should use ints for this?
- jumping: instead of applying contant upward velocity, could try reducing gravity while jump button held instead 
    - can also try the high parameter jump that sakurai suggested in his video

## Other:
- map: support tile rotations / flips? (leaning towards no)
