# To Do 

## Current Goal: 
- cleanup todos (pushing doesn't do callbacks, finish player anims, dynamic spawn points, consolidate collider components)

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
- EventFlow
    - execute a series of callbacks sequentially
    - max 1 callback per frame 
    - some callbacks may have a wait time

## Map:
- bake tile data into a mesh & use that for lighting
- should also give these tiles a Tile component that I can use for something like updating last safe point player was standing on

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
- rocket jumping state : updates rigidbody friction components (which may not exist) to have less friction or something
    - would work like this:
        - RocketJumping component added 
        - onAdd: update rigidbody vars, store old ones 
        - update: check if still rocket jumping & remove if done 
        - onRemove: restore old rigidbody data
- circular triggers

