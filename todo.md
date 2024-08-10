# To Do 

## Current Goal: 

## levels and game mechanics (each thing should have a level that teaches how to use)
- parachute
- air current (propeller?) -> comes before propeller

## Polish
- player leaning over ledge anim 
- crouching state/anim
- looking up anim
- aiming anim

## Player Controller/Abilities
- try working gravity multiplier into RJ state?
- in air: shooting projectile does have pushback? Maybe only if you shoot in a downward direction
- idea: downward-diagonal shooting pushes player slightly upward, making it more powerful
- idea: (small) double jump mechanic that kills your horizontal velocity, making it easier to adjust trajectory coming out of a rocket jump
    - could be a jump with neutral/directional variants for a little more control over what happens next
- ducking: make collider smaller, moves camera down after a sec like spelunky
- fast falling

## Camera / Follow
- pretty awful in general
- different movement types (easein/out stuff)
- be affected by momentum (maybe momentum should be added to velocity.total?)

## Map 
- respawn map objects
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
- if a tile overlaps one in a different layer, should only keep the one nearest to the camera? Would be nice for optimizations, but breaks down for something like foreground tiles?

## Misc
- ECS parallelization (low priority)
- ECS methods (get, set, remove) can't be run in debugger. Would at least like to have `.get<T>` working.
    - Sorta works now
- Logger queue that runs on another thread
- controller support (low priority)
- input remapping (saved to file too) (low priority)
- dialogue system (low priority)

---------------------------------------------------------------------------------------------------------------------------

# Research & Ideas
things i might want to (re)consider in the future -- ctrl+f for "RESEARCH" 

## System:
- make this an actual framework -- create GameInterface which has startup, mainloop, and end methods, then convert a lot of current Game class into Engine class which calls these things
- hot-reloading code (youtube video is bookmarked)

## Map:
- bake tile data into a mesh & use that for lighting
- serializing component structs into Tiled propertytypes would be cool, so I don't have to do so much work to add a new component, but it's probably not feasible bc edge cases

## Physics:
- jumping: instead of applying contant upward velocity, could try reducing gravity while jump button held instead 
    - can also try the high parameter jump that sakurai suggested in his video
- quad tree ray cast

## Other:
- should use 3rd party lib for Expected cause my impl sucks
- triggers which have some constraint, like X>=50

## ECS:
- the entity.set<T> problem (with IMonitor systems): it doesn't really make sense to handle the problem at the system level, because not all component modifications matter. If anything, could do an event callback for when a component is modified and let systems listen for specific component modifications

## Vector2T
- simd optimizations? (need profiling)
    - might make cross platform harder




Random note: how to save texture to image:
```cpp
auto filename = "TEST.png";
Image img = LoadImageFromTexture(tex.texture);
ExportImage(img, filename);
```
