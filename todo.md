# To Do 

## Current Goal: 
- player controller improvements
    - try working gravity multiplier into RJ state?
    - in air: shooting projectile does have pushback? Maybe only if you shoot in a downward direction
    - idea: downward-diagonal shooting pushes player slightly upward, making it more powerful
    - idea: (small) double jump mechanic that kills your horizontal velocity, making it easier to adjust trajectory coming out of a rocket jump
        - could be a jump with neutral/directional variants for a little more control over what happens next

## levels and game mechanics (each thing should have a level that teaches how to use)
- parachute
- air current (propeller?) -> comes before propeller

## Polish
- player leaning over ledge anim 

## Camera / Follow
- pretty awful in general
- different movement types (easein/out stuff)
- be affected by momentum (maybe momentum should be added to velocity.total?)

## Vector2T
- simd optimizations? (need profiling)

## Gfx
- replace radiance with bloom?
    - also want to try adding some translucent circle instead to see if that helps
    - anything w/ bloom would need to be drawn to its own texture, since it looks weird if bloom is constrained to only part of a canvas,
    but would need one texture per depth value? which would complicate things
        - better idea: draw loop goes like this:
            - create blank bloom texture 
            - while depth unchanged: if entity has bloom shader: draw to bloom texture (w/out bloom obviously)
            - if depth changes and bloom texture is not empty, then draw texture (w/ bloom shader on) && clear it for next depth value, otherwise do nothing

        - this makes me think bloom should be a tag, not a Draw shader? Since an entity could have a custom shader AND bloom & it would work
        - UPDATE: first implementation did not go well. Too messy w/ having to switch back to the target texture. Need a general-purpose draw-layer-to-texture-then-draw-layer-to-targettexture pipeline working before i try this

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

## Improving compile times:
- would be GREAT to use the <iosfwd> include in Print.h and move the iostream import outside of the header file, but not sure how to do that bc it's templated
- can maybe get <unordered_map> out of InputHandler.h and make it a static var in the source file
- static functions / anonymous namespaces for functions that are only defined/used in source files
- move CallbackMap definition from Physics.h to source file, make it static variable
    - Removing ECS include from System.h makes things slower for some reason

## Misc
- ECS parallelization (low priority)
- Logger queue that runs on another thread
- controller support (low priority)
- input remapping (saved to file too) (low priority)
- dialogue system (low priority)

---------------------------------------------------------------------------------------------------------------------------

# Research & Ideas
things i might want to (re)consider in the future -- ctrl+f for "RESEARCH" 

## System:
- make this an actual framework 
- hot-reloading code (youtube video is bookmarked)

## Map:
- bake tile data into a mesh & use that for lighting
- serializing component structs into Tiled propertytypes would be cool, so I don't have to do so much work to add a new component, but it's probably not feasible bc edge cases

## Physics:
- a lot of physics stuff (like velocity) is stored as floats even though it could be fixed precision (like nearest tenth of a texel) --> I should use ints for this?
- jumping: instead of applying contant upward velocity, could try reducing gravity while jump button held instead 
    - can also try the high parameter jump that sakurai suggested in his video
- quad tree ray cast

## Other:
- should use 3rd party lib for Expected cause my impl sucks
- triggers which have some constraint, like X>=50

## ECS:
- the entity.set<T> problem (with IMonitor systems): it doesn't really make sense to handle the problem at the system level, because not all component modifications matter. If anything, could do an event callback for when a component is modified and let systems listen for specific component modifications
