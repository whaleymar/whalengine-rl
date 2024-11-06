# To Do 

NEXT GOAL: PATH FINDING

## Separating Game vs Engine 
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
- Lights need Brightness multiplier so everything's not stuck in LDR
- fully in-shadow translucent objects are still visible, as well as text

## Lighting 
- issue with `iRender::draw` not being designed for outside shader use, but I'm using it for the occlusion depth + effect maps
    - instead of isPostProcessingUsed, have a separate `draw` call called `drawSilhouette` where you swearzies to not use a custom shader (and use a custom color)
- PointLight and BoxLight need to use Occlusion Depth map so they can't illuminate things closer to the camera than the light. Difficult because I draw them with UV schenanigans unlike ShadowLight

## Web 
- getting mouse position does not work

## Map 
- would like to do away with the default component function if possible
    - might be able to do this if I export the map project instead of saving https://discourse.mapeditor.org/t/is-it-possible-to-force-tiled-to-output-a-custom-property-even-when-default-value-is-selected/6272/6
    - one problem I'm having is with relative template paths -- I should keep the Tiled project in the Game's root directory to fix this (would make all paths easier to work with)
- the process of adding a new component is still annoying. Using a reflection library to improve that would be nice?
    - could have components inherit a ISerialize interface (`.save` and `.load` methods) && when the ECS world registers that component, it (via a registered `onComponentRegistered` callback) registers the component type (?) w/ some manager which maps the component name to the type, so when loading it can see the type name & dispatch the correct `.load` method, and when saving it can check if each component inherits the interface & call its `.save` method
    - i could make a component for tiled object ID and use that to know which objects to save
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
- Should use the Tiled collision editor for tile collision
- Instead of instancing each tile as an entity, levels should be an entity with a TileMap component which does the following:
    - non-animated tiles are drawn once to a RenderTexture (via its own system) on load, so a bunch of stuff doesn't have to be re-drawn every frame 
        - not sure how this would work with depth... maybe this only works with side-scroller games...
    - give it a SuperCollider/ParentCollider component which has all the individual tile colliders as ChildColliders
        - this might get slow if I do a linear search over Child colliders. I could use a QuadTree but instantiating that might also be slow...
    - animated tiles stay as their own entities, but are children of the level 

## Sprite Editing workflow
- .aseprite format support would be ideal. Could have some pre-compile step which unpacks the .ase files into PNGs, builds the atlas, then deletes the PNGs
    - see how Murder Engine does this

## Misc
- go all-in on custom raylib++ fork -> namespace the library and get rid of the bloat
- ECS parallelization (low priority)
- Logger queue that runs on another thread
- controller support (low priority)
- input remapping (saved to file too) (low priority)
- dialogue system (low priority)
- make physics simulation run at 60 fps even if framerate is higher
- ECS ISystem entities should be a vector, not a hashmap. Would improve cache locality & reduce memory usage. Any checks for if an entity is inside a system could be done by checking the entity's Pattern against the system's

---------------------------------------------------------------------------------------------------------------------------

# Research & Ideas
things i might want to (re)consider in the future -- ctrl+f for "RESEARCH" 

## System:
- hot-reloading code (youtube video is bookmarked)

## Map:
- bake tile data into a mesh & use that for lighting
- serializing component structs into Tiled propertytypes would be cool, so I don't have to do so much work to add a new component, but it's probably not feasible bc edge cases

## Other:
- should use 3rd party lib for Expected cause my impl sucks
- triggers which have some constraint, like X>=50

## ECS:
- the entity.set<T> problem (with IMonitor systems): it doesn't really make sense to handle the problem at the system level, because not all component modifications matter. If anything, could do an event callback for when a component is modified and let systems listen for specific component modifications

Random note: how to save texture to image:
```cpp
auto filename = "TEST.png";
Image img = LoadImageFromTexture(tex.texture);
ExportImage(img, filename);
```
