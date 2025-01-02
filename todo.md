# To Do 

NEXT GOAL: 
- want to independently toggle when an entity is inactive, regardless of its parent 
    - e.g. parent is inactive, child is "active", but parent's state overrides this so child is inactive 
    - e.g. parent is active, child is "inactive", so child is inactive 
    - e.g. parent is active, child is "active", so child is active
- fix hot reloading when loading reflect-cpp as a shared library 

## Separating Game vs Engine 
- want to edit CollisionLayers from Game 
- want to edit Input mappings and add custom Input Enum values from Game 
- editing globals like physics gravity/friction values from game -> put in Settings.cpp

## Components (some of these are duplicates of other tasks)
- BlocksAiPathing?
- dashed line
- parallax factor

## Gfx 
- need some sort of "root" Y sorting position that overrides actual position - like for particles that start below a column and float above it -- should look like they are consistently in front of or behind it
    - Float component works for this, but needs to be applied to tiles too for interaction to look correct
- animate the TileMapLayer overlay texture

## Lighting 
- PointLight and BoxLight need to use Occlusion Depth map so they can't illuminate things closer to the camera than the light. Difficult because I draw them with UV schenanigans unlike ShadowLight
    - consolidate pointlight and shadowlight
- shadows do NOT play well with a moving camera

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
- put tiled project in game's root directory so paths are easier to work with
    - this will let me export on save, which will fully resolve templates -> I can get rid of my shitty template code?
- respawn map objects
- refactor the Scene/Level hierarchy to use entities
    - (maybe) keep scenes as is, but levels could be entities

## Sprite Editing workflow
- .aseprite format support would be ideal. Could have some pre-compile step which unpacks the .ase files into PNGs, builds the atlas, then deletes the PNGs
    - see how Murder Engine does this

## Camera 
- follow a spline whose points are defined in the level.
    - can use this to find the closest point on a spline to the player: https://homepage.math.uiowa.edu/~atkinson/ftp/CurvesAndSufacesClosestPoint.pdf

## Physics 
- colliders need to scale with transform
    - kinda hard because the physics system only cares about position. There's nothing checking if a collider's size matches the scale

## Rendering Performance (if needed)
- put sorting on a 1 frame delay and have a separate thread sort entities from the previous frame. 
    - delay would be minimal. Only affects newly created/deleted entities + entites which just walked in front/behind something
- one of these:
    - have render systems maintain their entities sorted, then can merge using std::merge
    - cache entity positions from previous frame, remove and re-insert entities whose positions changed

## Misc
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
