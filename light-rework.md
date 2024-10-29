1. make list of everything that blocks light
2. order the list by depth
3. draw the occlusion map using this. It needs to use the depth buffer for a later step.
4. make list of everything that casts light
5. sort the light entities
6. draw the lights w/ their depth information
    7. shadowlights should work as-is
    8. point lights and box lights should use the occlusion map's depth buffer to check whether they're **behind** an occluder
    9. if they are, don't draw that pixel



ALTERNATIVE

*lights are now drawn with irender*
- pros: 
    - no need for depth buffer shit
    - same interface as normal drawing

- cons:
    - depth sorting would need to be a bit different, since if an entity has a sprite + light then the light should illuminate that, but in practice the sprite would block the light, so I'd have to artificially add depth to light to match its sprite (if it has one?)
    - can't apply custom filters (like blur) to light map. Lighting and Game objects are all rendered to the same texture
    - have to draw lights at full resolution :(

