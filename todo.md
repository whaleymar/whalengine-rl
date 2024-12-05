# To Do 

NEXT GOAL: SERIALIZATION
- convert entity factory into new version

## Separating Game vs Engine 
- want to edit CollisionLayers from Game 
- want to edit Input mappings and add custom Input Enum values from Game 
- editing globals like physics gravity/friction values from game -> put in Settings.cpp

## Components (some of these are duplicates of other tasks)
- BoxLight in map 
- BlocksAiPathing?
- dashed line
- parallax factor
- SpriteVisibilityMask -- a standalone sprite which affects visibility of main Sprite component

## Gfx 
- use sprite masking to control which parts of a sprite are bloomed: https://youtu.be/WiDVoj5VQ4c?si=kd5caB1nMtbDYr7v
    - would apply to sprite color, not necessarily bloom. Can use it as a transparency mask, brightness mask, etc.
- need some sort of "root" Y sorting position that overrides actual position - like for particles that start below a column and float above it -- should look like they are consistently in front of or behind it
- tile performance: can put tile Sprite components in a shared LUT and store index in tile component?
    - can also write a faster variant of getDrawParams that omits unused stuff like rotation/scaling
        - should also try passing trans as a reference in those funcs
- posterization shader doesn't work right with HDR colors

## Lighting 
- PointLight and BoxLight need to use Occlusion Depth map so they can't illuminate things closer to the camera than the light. Difficult because I draw them with UV schenanigans unlike ShadowLight
    - consolidate pointlight and shadowlight
- follow this tutorial to properly generate a Signed Distance Field, which will make shadows much faster
    - https://jason.today/gi 

## Web 
- getting mouse position does not work (may be fixed w/ raylib 5.5)
- need to update a lot of shaders 

## Debug tools 
- imgui integration
    - want to click on an entity and have access to all of its components & their values & be able to change them dynamically
    - change which Scene I'm in -- allows for debug-only scenes that are easier to use
    - toggle which shaders are used in the camera's pipeline

## Triggers 
- consolidate with colliders like unity. Makes a lot less work :) 
    - would need to finally add non-aabb shapes to collider though
    - i'm thinking QuadTree stays exactly the same (AABB only) and there's an extra isOverlapping step that non-aabb shapes have to do post-query

## Map 
- would like to do away with the default component function if possible
    - might be able to do this if I export the map project instead of saving https://discourse.mapeditor.org/t/is-it-possible-to-force-tiled-to-output-a-custom-property-even-when-default-value-is-selected/6272/6
    - one problem I'm having is with relative template paths -- I should keep the Tiled project in the Game's root directory to fix this (would make all paths easier to work with)
- the process of adding a new component is still annoying. Using a reflection library to improve that would be nice?
    - i could make a component for tiled object ID and use that to know which objects to save
- respawn map objects
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

## Sprite Editing workflow
- .aseprite format support would be ideal. Could have some pre-compile step which unpacks the .ase files into PNGs, builds the atlas, then deletes the PNGs
    - see how Murder Engine does this

## Camera 
- follow a spline whose points are defined in the level.
    - can use this to find the closest point on a spline to the player: https://homepage.math.uiowa.edu/~atkinson/ftp/CurvesAndSufacesClosestPoint.pdf

## Physics 
- colliders need to scale with transform
    - kinda hard because the physics system only cares about position. There's nothing checking if a collider's size matches the scale

## Misc
- define some common tween functions (transform position, rotation, sprite scale, etc) in a header
- rich text support: https://docs.unity3d.com/Packages/com.unity.ugui@1.0/manual/StyledText.html
    - also want to support tags for effects, like the text moving in a wave pattern
- invisibility tag should affect children

## Misc (low priority)
- ECS parallelization (low priority)
- controller support (low priority)
- input remapping (saved to file too) (low priority)
- make physics simulation run at 60 fps even if framerate is higher
- ECS ISystem entities should be a vector, not a hashmap. Would improve cache locality & reduce memory usage. Any checks for if an entity is inside a system could be done by checking the entity's Pattern against the system's
- Get web and windows builds working again
- if I ever want a multi-camera setup, each camera would need its own RenderTexture::Main to draw to.

---------------------------------------------------------------------------------------------------------------------------

# Research & Ideas
things i might want to (re)consider in the future -- ctrl+f for "RESEARCH" 

## Other:
- should use 3rd party lib for Expected cause my impl sucks

Random note: how to save texture to image:
```cpp
auto filename = "TEST.png";
Image img = LoadImageFromTexture(tex.texture);
ExportImage(img, filename);
```
