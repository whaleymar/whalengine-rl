# To Do 

## Current Goal: 

## Camera / Follow
- different movement types (easein/out stuff)
- be affected by momentum (maybe momentum should be added to velocity.total?)

## Systems
- lighting\*
- ui -- try building pause menu which\*
    1. is triggered with a pause event 
    2. makes music quieter
    3. pauses all sfx (minus one channel used in pause menu)
    4. creates simple text gui : {Resume, Restart, Quit} -- each of these send their own events
    - maybe just do bitmap font with instanced letters?

## Components
- Light 
- Attached (relationship)
- EventFlow
    - execute a series of callbacks sequentially
    - max 1 callback per frame 
    - some callbacks may have a wait time

## Entity Prefabs
- particle
    - draw(?), position, velocity, lifetime
- death plane

## Physics
- idea: overlapping actors/semisolids nudge each other away or exert a force or something
- low priority: actors always check for a collision with a solid before a semisolid, so semisolid callbacks don't always run. Could be fixed by storing solids and semisolid pointers together for these collision checks, but it's not a huge deal
- actor collision callbacks don't run if they're pushed
- solid/semisolid callbacks don't run if they're pushing
    - --> should have some onPush[ed] callbacks for these cases i guess? bc not all callbacks should run when pushing

## Map 
- support rotations / flips? (leaning towards no)
- object layers
    - component factory functions (mostly done)
    - special metadata
        - spawn points
        - camera strat (might want to rework)
    - Templates
- parallax

## Graphics
- outline shader
- color quantization shader
- repeating texture

## Non-ECS Systems
- chunk loading/unloading (quad tree?)

## Misc
- ECS lib tasks
- make this an actual framework 
- Logger queue that runs on another thread
- rework controller system to be manually called by Event
- hot-reloading code (youtube video is bookmarked)
- a lot of physics stuff (like velocity) is stored as floats even though it could be fixed precision (like nearest tenth of a texel) --> I should use ints for this? #CLEANUP
- the thing that checks what level i'm in is based on left edge of transform instead of the center?

## Bugs
- isNearZero not working
- cppcheck issues
- actors sometimes fall through one way solids if the solid is moving fast enough, probably because solids don't move one pixel at a time, so if the actor isn't already riding the solid, it misses the solid's boundary

## Research
- things i might want to reconsider in the future -- ctrl+f for "RESEARCH" 

