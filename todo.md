# To Do 

ACTIVELY WORKING ON:

## Separating Game vs Engine 
- want to edit CollisionLayers from Game 
- editing globals like physics gravity/friction values from game -> put in Settings.cpp
- Want to create more world materials in game

## Components (some of these are duplicates of other tasks)
- Many components have an `offset` vector which can be replaced with a child entity. I'll need to handle this in loadImpls and create a new entity when there's an offset. This might break some EntityFactory code.
    - collider (probably keep this one)
    - trigger 
    - particle emitter
    - lights

## Gfx 
- need some sort of "root" Y sorting position that overrides actual position - like for particles that start below a column and float above it -- should look like they are consistently in front of or behind it
    - Float component works for this, but needs to be applied to tiles too for interaction to look correct
- animate the TileMapLayer overlay texture
- use the balatro sfx pattern for loading shaders -> at startup load every in a dedicated 'shaders' folder into memory and allow them to be queried globally with a string key instead of an enum 

## Lighting 
- PointLight and BoxLight need to use Occlusion Depth map so they can't illuminate things closer to the camera than the light. Difficult because I draw them with UV schenanigans unlike ShadowLight
    - consolidate pointlight and shadowlight
- shadows do NOT play well with a moving camera (due to pixel clamping)
- when shadowlight entity is floating, should do a raycast to ensure the lighting isn't appearing on the other side of walls

## Web 

## Debug tools 
- imgui integration
    - add more components to imgui component renderer
    - change which Scene I'm in -- allows for debug-only scenes that are easier to use
    - toggle which shaders are used in the camera's pipeline
    - search for entity by name, inspect in editor

## Save data
- figure out how to serialize callbacks (like onDeath component, Lifetime::onDeath, Collider::onCollisionEnter, Trigger::xyz)
    - could integrate lua scripting and write the callbacks using those
        - lua integration is doable (and brings benefits like insta hot reloading) 
        - but would require extending ComponentFactory to convert components to/from lua tables
    - could stop using lambdas and exclusively use named free functions. Then I can serialize the function signature

## Map 
- put tiled project in game's `data` directory so paths are easier to work with
    - this will let me export on save, which will fully resolve templates -> I can get rid of my shitty template code?
- respawn map objects
- refactor the Scene/Level hierarchy to use entities
    - (maybe) keep scenes as is, but levels could be entities

## Camera 
- follow a spline whose points are defined in the level.
    - can use this to find the closest point on a spline to the player: https://homepage.math.uiowa.edu/~atkinson/ftp/CurvesAndSufacesClosestPoint.pdf

## Physics 
- colliders need to scale with transform
    - kinda hard because the physics system only cares about position. There's nothing checking if a collider's size matches the scale

## Rendering Performance (if needed)
- put sorting on a 1 frame delay and have a separate thread sort entities from the previous frame. 
    - delay would be minimal. Only affects newly created/deleted entities + entites which just walked in front/behind something
- cache entity positions from previous frame, remove and re-insert entities whose positions changed

## Misc
- rich text support: https://docs.unity3d.com/Packages/com.unity.ugui@1.0/manual/StyledText.html
    - also want to support tags for effects, like the text moving in a wave pattern
- invisibility tag should affect children
- want to independently toggle when an entity is inactive, regardless of its parent 
    - e.g. parent is inactive, child is "active", but parent's state overrides this so child is inactive 
    - e.g. parent is active, child is "inactive", so child is inactive 
    - e.g. parent is active, child is "active", so child is active
    - *example use case*: level object that is inactive until some event happens. 
        - Currently I am making portals invisible + an empty layer mask + a CustomUpdate that checks the enemy count every frame & changes the values when it "activates"
        - that last part is annoying cause I have to recursively remove the invisible tag in children, change the particleemitter params, and change the layer mask
        - ideally there is a parent with the CustomUpdate method, but once the enemy count condition is met, I just activate a child entity holding the portal components
- Sfx pitch modulation

## Misc (low priority)
- ECS parallelization (low priority)
- make physics simulation run at 60 fps even if framerate is higher
- ECS ISystem entities should be a vector, not a hashmap. Would improve cache locality & reduce memory usage. Any checks for if an entity is inside a system could be done by checking the entity's Pattern against the system's
- Get windows builds working again
- if I ever want a multi-camera setup, each camera would need its own RenderTexture::Main to draw to.

## Bugs 
- quad bleeding -- can only be fixed by adding padding between sprites in sprite atlas
- hot reloading texture atlas is broken? at least when refreshing animation frame times

---------------------------------------------------------------------------------------------------------------------------

# Research & Ideas
things i might want to (re)consider in the future -- ctrl+f for "RESEARCH" 

## Other:
- should use 3rd party lib for Expected cause my impl sucks
