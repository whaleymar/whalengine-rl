# To Do 

## Current Goal: 

## Camera / Follow
- different movement types (easein/out stuff)
- be affected by momentum (maybe momentum should be added to velocity.total?)

## Components
- Light 
- AudioListener - give to camera, add some debug assert to make sure there's only ever one 
    - used as target for 3D/spatial audio

## Entity Prefabs
- particle
    - draw(?), position, velocity, lifetime
- death plane
- read prefab component values from YAML
    - combined w/ hot reloading, would make fine tuning prototypes easier

## Physics
- actor collision callbacks don't run if they're pushed
- solid/semisolid callbacks don't run if they're pushing
    - --> should have some onPush[ed] callbacks for these cases i guess? bc not all callbacks should run when pushing
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
- color quantization shader

## Misc
- ECS lib tasks
- Logger queue that runs on another thread
- rework controller system to be manually called by Event
    - better way to remove player input: an event listener which has higher priority and can stop other listeners for the same event for running. 
- put camera transform in screen precision coordinates to reduce jiggle ? (saint11 blog about it)

## Bugs
- isNearZero not working
- cppcheck issues
- actors sometimes fall through one way solids if the solid is moving fast enough, probably because solids don't move one pixel at a time, so if the actor isn't already riding the solid, it misses the solid's boundary
    - it almost never happens though

---------------------------------------------------------------------------------------------------------------------------

# Research & Ideas
things i might want to (re)consider in the future -- ctrl+f for "RESEARCH" 

## System:
- make this an actual framework 
- hot-reloading code (youtube video is bookmarked)
- EventFlow
    - execute a series of callbacks sequentially
    - max 1 callback per frame 
    - some callbacks may have a wait time

## Physics:
- a lot of physics stuff (like velocity) is stored as floats even though it could be fixed precision (like nearest tenth of a texel) --> I should use ints for this?
- jumping: instead of applying contant upward velocity, could try reducing gravity while jump button held instead 
    - can also try the high parameter jump that sakurai suggested in his video
- overlapping actors/semisolids nudge each other away or exert a force or something
- low priority: actors always check for a collision with a solid before a semisolid, so semisolid callbacks don't always run. Could be fixed by storing solids and semisolid pointers together for these collision checks, but it's not a huge deal
- rewrite: a simplified collision component with:
    - a layer (actor, solid, semisolid, lightblocking, etc?)
    - a shape (maybe one day)
    - https://github.com/isadorasophia/murder/tree/main has an interesting approach

## Other:
- map: support tile rotations / flips? (leaning towards no)
