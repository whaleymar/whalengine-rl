# To Do 

NEXT GOAL: 
- HDR drawTexture method
- HDR color struct

## Separating Game vs Engine 
- want to edit CollisionLayers from Game 
- want to edit Input mappings and add custom Input Enum values from Game 
- editing globals like physics gravity/friction values from game -> put in Settings.cpp

## Bugs 
- use after free in Audio module. No guarantee that AudioPlayer destructor runs after all sounds are free'd so sounds can try to access audioplayer after its been destroyed.

## Components (some of these are duplicates of other tasks)
- BoxLight in map 
- BlocksAiPathing?
- dashed line
- parallax factor
- SpriteVisibilityMask -- a standalone sprite which affects visibility of main Sprite component

## Gfx 
- use sprite masking to control which parts of a sprite are bloomed: https://youtu.be/WiDVoj5VQ4c?si=kd5caB1nMtbDYr7v
- Lights need Brightness multiplier so everything's not stuck in LDR
- need some sort of "root" Y sorting position that overrides actual position - like for particles that start below a column and float above it -- should look like they are consistently in front of or behind it
- tile performance: can put tile Sprite components in a shared LUT and store index in tile component?

## Lighting 
- PointLight and BoxLight need to use Occlusion Depth map so they can't illuminate things closer to the camera than the light. Difficult because I draw them with UV schenanigans unlike ShadowLight
    - consolidate pointlight and shadowlight

## Web 
- getting mouse position does not work (may be fixed w/ raylib 5.5)
- need to update a lot of shaders 

## Debug tools 
- imgui integration
    - want to click on an entity and have access to all of its components & their values & be able to change them dynamically
    - changing shader uniforms dynamically would also be cool
    - change which RenderTexture I'm looking at 
    - change which Scene I'm in -- allows for debug-only scenes that are easier to use

## Triggers 
- consolidate with colliders like unity. Makes a lot less work :) 

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

## Camera 
- follow a spline whose points are defined in the level.
    - can use this to find the closest point on a spline to the player: https://homepage.math.uiowa.edu/~atkinson/ftp/CurvesAndSufacesClosestPoint.pdf

## Prefab Injection 
- want to auto-register prefab classes without having to manually add everything to a static list like I do now 
- considering using the Static Initialization / Inversion of Control pattern used by Google's test suite (gtest)
    - github.com/google/googletest/blob/main/googletest/include/gtest/gtest.h
    - https://www.cppstories.com/2023/ub-factory-constinit/ -- cpp20 approach (?)

## Physics 
- collider offsets + rotation + scaling doesn't work quite right

## Misc
- ECS parallelization (low priority)
- controller support (low priority)
- input remapping (saved to file too) (low priority)
- make physics simulation run at 60 fps even if framerate is higher
- ECS ISystem entities should be a vector, not a hashmap. Would improve cache locality & reduce memory usage. Any checks for if an entity is inside a system could be done by checking the entity's Pattern against the system's
- Possible Shader workflow: Unity Shader Graph -> view code -> export code as GLSL?
- define some common tween functions (transform position, rotation, sprite scale, etc) in a header
- should collider size change with scale? that's how it works in unity
    - kinda hard because the physics system only cares about position. There's nothing checking if a collider's size matches the scale
- if I ever want a multi-camera setup, each camera would need its own RenderTexture::Main to draw to.
- Get web and windows builds working again
- rich text support: https://docs.unity3d.com/Packages/com.unity.ugui@1.0/manual/StyledText.html
    - also want to support tags for effects, like the text moving in a wave pattern
- invisibility tag should affect children

---------------------------------------------------------------------------------------------------------------------------

# Research & Ideas
things i might want to (re)consider in the future -- ctrl+f for "RESEARCH" 

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
