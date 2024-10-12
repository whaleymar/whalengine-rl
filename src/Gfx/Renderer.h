#pragma once

typedef struct Camera2D Camera2D;

namespace whal {

// does a few things (that I might break up)
// 1. ECS systems with draw-like methods are updated (this should probably happen automatically)
// 2. Renders all game objects and lighting onto the TextureID::Main RenderTexture
// 3. (maybe?) applies post-processing effects
// 4. Renders debug information, like colliders (if applicable)
void render(Camera2D worldCamera);

}  // namespace whal
