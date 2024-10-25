# To Do 

## Separating Game vs Engine 
- !!! anything that's static and not const could cause a crash and needs to be reset on game unload
- want to edit CollisionLayers from Game 
- want to edit Input mappings and add custom Input Enum values from Game 
- editing globals like physics gravity/friction values from game would be nice, but that might balloon compile times (maybe I can use extern?)
- Settings.h -> put in game? Or don't define anything && put the .cpp file in Game/ ?

## Cleanup
- consistent System naming (files and classes)
    - "XyzSystem"
- Put all systems in the Systems/ dir. Some are in component files
- One file per system

## Components (some of these are duplicates of other tasks)
- BoxLight in map 
- BlocksAiPathing?
- dashed line
- parallax factor

## Gfx 
- like godot, should have tag components for {Blocks light (DONE), Blocks AI pathing}
- bloom shader is a little broken (reloading makes it look wildly different)
    - kinda want to get rid of my fake bloom entirely and implement HDR + tone mapping

## Lighting 
- shouldn't be able to illuminate things that are closer to camera than the light -- is possible right now because we draw everything and *then* draw the light
    - but i can't just draw the light to the main tex after each Depth layer, because then some things which would be lit by foreground would be dark
- player sprite should NOT affect shadows!!!

## Web 
- getting mouse position does not work

## Map 
- would like to do away with the default component function if possible
- the process of adding a new component is still annoying. Using a reflection library to improve that would be nice.
- things not on the tile grid have their collision/trigger boxes messed up
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
- reflect-cpp string -> enum transformations when parsing Tiled map data 

## Sprite Editing workflow
- .aseprite format support would be ideal. Could have some pre-compile step which unpacks the .ase files into PNGs, builds the atlas, then deletes the PNGs
    - see how Murder Engine does this

## Misc
- ECS parallelization (low priority)
- Logger queue that runs on another thread
- controller support (low priority)
- input remapping (saved to file too) (low priority)
- dialogue system (low priority)
- make physics simulation run at 60 fps even if framerate is higher

---------------------------------------------------------------------------------------------------------------------------

# Research & Ideas
things i might want to (re)consider in the future -- ctrl+f for "RESEARCH" 

## System:
- hot-reloading code (youtube video is bookmarked)

## Map:
- bake tile data into a mesh & use that for lighting
- serializing component structs into Tiled propertytypes would be cool, so I don't have to do so much work to add a new component, but it's probably not feasible bc edge cases

## Physics:
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
