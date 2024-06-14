# To Do 

## Current Goal: 
- collider consolidation:
    - callback prioritization
    - misc TODOs
- cleanup todos (finish player anims, dynamic spawn points)

## Controls 
- try making controls more like towerfall

## Camera / Follow
- different movement types (easein/out stuff)
- be affected by momentum (maybe momentum should be added to velocity.total?)

## Components
- Light 

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
        - dynamic spawn points
        - camera strat (might want to rework)
    - Templates

## Graphics
- outline shader

## Improving compile times:
- make ECS shared pointers unique pointers which return raw pointers (callers don't need ownership)
    - then switch to `CorradePointer.h` for a drop-in replacement for unique_ptr which apparently makes compile times a lot better
- would be GREAT to use the <iosfwd> include in Print.h and move the iostream import outside of the header file, but not sure how to do that bc it's templated
- can maybe get <optional> out of Audio.h 
- can maybe get <unordered_map> out of InputHandler.h and make it a static var in the source file
- `CorradeOptional.h`
- instead of including Vector everywhere, can do `extern template struct Vector2T<s32>` ??? (idk how to do the typedef part)
- static functions / anonymous namespaces for functions that are only defined/used in source files
- remove <memory> include in Tiled.h
- would like to remove ECS include in Relationships.h
    - also HitInfo
    - same for System.h, where I can grab the ECS ptr using a function that's defined in System's source file?

    - making me think i need an ECS_fwd file or something, cause these things just need to know the size of an entity, but I could also just store an entity id? And then give ECS a getEntityByID func or something


## Misc
- ECS lib tasks
- Logger queue that runs on another thread
- rework controller system to be manually called by Event
    - better way to remove player input: an event listener which has higher priority and can stop other listeners for the same event for running. 
- put camera transform in screen precision coordinates to reduce jiggle ? (saint11 blog about it)

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
- low priority: actors always check for a collision with a solid before a semisolid, so semisolid callbacks don't always run. Could be fixed by storing solids and semisolid pointers together for these collision checks, but it's not a huge deal

## Other:
- map: support tile rotations / flips? (leaning towards no)
- circular triggers

