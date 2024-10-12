#include "Renderer.h"

#include <raylib.h>

#include "Gfx/Pipeline.h"
#include "Gfx/Texture.h"

#include "Sys/System.h"

#include "Systems/CollisionManager.h"
#include "Systems/Gfx.h"
#include "Systems/LightSystem.h"

#include "Util/EngineUtil.h"

namespace whal {

void render(Camera2D worldCamera) {
    // 1. ECS systems with draw-like methods are updated (this should probably happen automatically)
    System::world.getSystem<RadianceLightSystem>()->drawEntities(worldCamera);  // drawn to TextureID::Radiance
    TextureManager::instance().renderBackgroundTextures();                      // drawn to TextureID::Background
    System::world.getSystem<GfxSystem>()->drawEntities(worldCamera);            // draws entities AND backgrounds to TextureID::Main
    drawLights(worldCamera);                                                    // drawn to TextureID::UpscaledLighting

    // 2. Renders everything to TextureID::Main
    BeginTextureMode(TextureManager::getRenderTexture(TextureID::Main));
    ClearBackground(BLACK);

    // Game Objects.
    drawRenderTexture(TextureManager::getRenderTexture(TextureID::Staging));

    // testing
    // drawRenderTexture(TextureManager::getRenderTexture(TextureID::Occlusion));

    // Lights.
    BeginBlendMode(BLEND_MULTIPLIED);
    drawRenderTexture(TextureManager::getRenderTexture(TextureID::UpscaledLighting));
    EndBlendMode();

    // Radiance. Is not upscaled.
    RenderTexture2D radianceTexture = TextureManager::getRenderTexture(TextureID::Radiance);
    Rectangle screenSourceRec =
        Rectangle(0.0f, 0.0f, static_cast<f32>(radianceTexture.texture.width), -static_cast<f32>(radianceTexture.texture.height));
    Rectangle dstRect(0, 0, radianceTexture.texture.width * VIRTUAL_SCREEN_RATIO, radianceTexture.texture.height * VIRTUAL_SCREEN_RATIO);
    BeginBlendMode(BLEND_ADDITIVE);
    DrawTexturePro(radianceTexture.texture, screenSourceRec, dstRect, {0.0f, 0.0f}, 0.0f, WHITE);
    EndBlendMode();

    // Text. *Could* be drawn with other game objects, but that might make UI annoying.
    System::world.getSystem<DrawTextSystem>()->drawEntities(WHITE);
    EndTextureMode();

    // 3. ? Apply post processing
    // TODO this should be accessible by the game
    static Pipeline postProcessPipeline = Pipeline({WINDOW_WIDTH_ACTUAL, WINDOW_HEIGHT_ACTUAL}, {
                                                                                                    // Shaders::Glitch,
                                                                                                    // Shaders::Quantize,
                                                                                                });
    postProcessPipeline.process(TextureID::Main);

    // 4. Draw debug stuff.
    // TODO UI stuff would also go here (without the ifndef obvi)
#ifndef NDEBUG
    if (System::input.isOn(InputType::DEBUG)) {
        BeginTextureMode(TextureManager::getRenderTexture(TextureID::Main));
        BeginMode2D(worldCamera);
        // System::world.getSystem<DrawDebugSystem>()->drawEntities();
        drawColliders();
        EndMode2D();
        EndTextureMode();
    }
#endif
}

//
// RELIC: some tone mapping code I wrote for radiance:
//
// static int exposureUniform = GetShaderLocation(ShaderManager::get(Shaders::ToneMap), "exposure");
// static float exposure = 1.0;
//
// if (IsKeyPressed(KEY_UP)) {
//     exposure += 0.1;
//     print("exposure: ", exposure);
// } else if (IsKeyPressed(KEY_DOWN)) {
//     exposure -= 0.1;
//     print("exposure: ", exposure);
// }

// <draw radiance texture>

// auto shader = ShaderManager::get(Shaders::ToneMap);
// BeginShaderMode(shader);
// SetShaderValue(shader, exposureUniform, &exposure, SHADER_UNIFORM_FLOAT);
// DrawTexture(getRenderTexture(TextureID::Main).texture, 0, 0, WHITE);
// EndShaderMode();

}  // namespace whal
