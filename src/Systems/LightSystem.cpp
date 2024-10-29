#include "LightSystem.h"

#include <cmath>
#include <raylib.h>
#include <rlgl.h>

#include "Components/Light.h"
#include "Components/Tags.h"
#include "Components/Transform.h"

#include "Events/Events.h"
#include "Gfx/Coordinates.h"
#include "Gfx/Pipeline.h"
#include "Gfx/RaylibUtil.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Settings.h"
#include "Sys/System.h"
#include "Systems/TagTrackers.h"

#include "Util/Color.h"
#include "Util/Easing.h"
#include "Util/Vector.h"

namespace whal {

const Color COLOR_AMBIENT = Color(0, 0, 0, 255);

void drawLights(Camera2D worldCamera) {
    const auto lightTex = TextureManager::getRenderTexture(TextureID::Lighting);
    BeginTextureMode(lightTex);
    BeginMode2D(worldCamera);
    ClearBackground(COLOR_AMBIENT);

    BeginBlendMode(BLEND_ADDITIVE);
    System::world.getSystem<PointLightSystem>()->drawEntities();
    System::world.getSystem<BoxLightSystem>()->drawEntities();
    EndMode2D();

    System::world.getSystem<ShadowLightSystem>()->drawEntities();
    EndBlendMode();
    EndTextureMode();

    // blur the lighting texture
    static const Pipeline lightingPipeline({WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME}, {Shaders::BlurLowRes});
    lightingPipeline.process(TextureID::Lighting);

    // upscale the lighting to full resolution
    const auto lightTexUpscale = TextureManager::getRenderTexture(TextureID::UpscaledLighting);
    BeginTextureMode(lightTexUpscale);

    const Rectangle srcRect = Rectangle(0, 0, lightTex.texture.width, -lightTex.texture.height);
    const Rectangle dstRect = Rectangle(0, 0, lightTexUpscale.texture.width, lightTexUpscale.texture.height);
    DrawTexturePro(lightTex.texture, srcRect, dstRect, Vector2{0, 0}, 0.0f, WHITE);

    EndTextureMode();
}

// RESEARCH this assumes the entity is rotated about the transform position
static Vector2i getLightOffset(f32 rotationDegrees, s32 lightHeightPixels) {
    return (angleToUnit(-rotationDegrees + 90.0f) * lightHeightPixels).round();
}

void PointLightSystem::onEvent(ShaderReloadEvent) {
    mPositionUniform = GetShaderLocation(ShaderManager::get(Shaders::PointLight), "position");
}

void PointLightSystem::drawEntities() {
    auto cameraPos = getCameraPositionPrecise();
    // auto cameraPos = toFloatVec(getCameraPosition());

    Shader shader = ShaderManager::get(Shaders::PointLight);
    ScopedShader shaderScope = ShaderManager::activateScoped(Shaders::PointLight);

    const Texture& randomTexture = TextureManager::getAtlas(TEXNAME_SPRITE).getTexture();
    for (auto [entityid, entity] : getEntitiesMutable()) {
        if (entity.has<Invisible>()) {
            continue;
        }
        PointLight light = entity.get<PointLight>();
        const auto trans = entity.get<Transform2D>();
        const Vector2i worldPosition = trans.position + getLightOffset(trans.rotationDegrees, light.heightOffset);
        const Vector2i screenPosition =
            Vector2i(worldPosition.x - cameraPos.x, -1 * worldPosition.y + cameraPos.y) + Vector2i(WINDOW_WIDTH_GAME / 2, WINDOW_HEIGHT_GAME / 2);
        Color color = Color(light.color.r, light.color.b, light.color.g, light.color.a);

        // RESEARCH may want to put this as a param in the component
        constexpr f32 intensity = 1.0;
        s32 radius = light.radius;

        color.a = std::lerp(COLOR_AMBIENT.a, color.a, intensity);
        radius = ease(radius / 2, radius, intensity, Ease::InQuad);

        Vector2 screenPosV(screenPosition.x, screenPosition.y);
        SetShaderValue(shader, mPositionUniform, &screenPosV, SHADER_UNIFORM_VEC2);

        const Rectangle srcRect(0, 0, randomTexture.width, randomTexture.height);
        const Rectangle dstRect(screenPosition.x - radius, screenPosition.y - radius, radius * 2, radius * 2);
        DrawTexturePro(randomTexture, srcRect, dstRect, Vector2(0, 0), 0, color);
        // DrawTextureDepth(randomTexture, srcRect, dstRect, Vector2(0, 0), 0, color, depthToFloat(trans.depth));
    }
}

void BoxLightSystem::onEvent(ShaderReloadEvent) {
    Shader shader = ShaderManager::get(Shaders::BoxLight);
    mPositionUniform = GetShaderLocation(shader, "lightpos");
    mHalflenUniform = GetShaderLocation(shader, "lighthalflen");
    mRadiusUniform = GetShaderLocation(shader, "lightradius");
}

void BoxLightSystem::drawEntities() {
    auto cameraPos = getCameraPositionPrecise();
    Shader shader = ShaderManager::get(Shaders::BoxLight);

    Texture randomTexture = TextureManager::getRenderTexture(TextureID::Lighting).texture;
    for (auto [entityid, entity] : getEntitiesMutable()) {
        if (entity.has<Invisible>()) {
            continue;
        }

        // do this every entity so draw calls aren't instanced and the uniform changes
        // should be fine if there aren't a ton of these lights
        ScopedShader shaderScope = ShaderManager::activateScoped(Shaders::BoxLight);

        BoxLight light = entity.get<BoxLight>();
        const Vector2i worldPosition =
            entity.get<Transform2D>().position + getLightOffset(entity.get<Transform2D>().rotationDegrees, light.heightOffset);
        // Vector2i screenPosition(worldPosition.x - cameraPos.x, -1 * worldPosition.y + cameraPos.y); // OLD
        Vector2i screenPosition =
            Vector2i(worldPosition.x - cameraPos.x, -1 * worldPosition.y + cameraPos.y) + Vector2i(WINDOW_WIDTH_GAME / 2, WINDOW_HEIGHT_GAME / 2);
        Color color = Color(light.color.r, light.color.b, light.color.g, light.color.a);

        // RESEARCH may want to put this as a param in the component
        constexpr f32 intensity = 1.0;
        s32 radius = light.radius;

        // apply fading
        color = Colors::lerp(COLOR_AMBIENT, color, intensity);

        // light falls off quadratically
        radius = ease(radius / 2, radius, intensity, Ease::InQuad);

        Vector2 screenPosV(screenPosition.x, screenPosition.y);
        Vector2 halfLenV(light.halfLen.x, light.halfLen.y);
        f32 fRadius = static_cast<f32>(radius);
        SetShaderValue(shader, mPositionUniform, &screenPosV.x, SHADER_UNIFORM_VEC2);
        SetShaderValue(shader, mHalflenUniform, &halfLenV.x, SHADER_UNIFORM_VEC2);
        SetShaderValue(shader, mRadiusUniform, &fRadius, SHADER_UNIFORM_FLOAT);

        const Vector2i lightBounds(radius + light.halfLen.x, radius + light.halfLen.y);
        const Vector2i destPosition = screenPosition - lightBounds;
        const Vector2i destSize = lightBounds * 2;

        const Rectangle srcRect(0, 0, randomTexture.width, randomTexture.height);
        const Rectangle dstRect(destPosition.x, destPosition.y, destSize.x, destSize.y);

        DrawTexturePro(randomTexture, srcRect, dstRect, Vector2(0, 0), 0, color);
    }
}

void RadianceLightSystem::onEvent(ShaderReloadEvent) {
    mPositionUniform = GetShaderLocation(ShaderManager::get(Shaders::Radiance), "position");
}

void RadianceLightSystem::drawEntities(Camera2D worldCamera) {
    auto cameraPos = getCameraPositionPrecise();
    // auto cameraPos = toFloatVec(getCameraPosition());

    Shader shader = ShaderManager::get(Shaders::Radiance);
    ScopedShader shaderScope = ShaderManager::activateScoped(Shaders::Radiance);
    BeginTextureMode(TextureManager::getRenderTexture(TextureID::Radiance));
    BeginMode2D(worldCamera);

    ClearBackground({0, 0, 0, 0});  // don't overwrite background stuff

    const Texture randomTexture = TextureManager::getRenderTexture(TextureID::DownscaledPostProcess).texture;
    for (auto [entityid, entity] : getEntitiesMutable()) {
        if (entity.has<Invisible>()) {
            continue;
        }

        Radiance light = entity.get<Radiance>();
        const Vector2i worldPosition =
            entity.get<Transform2D>().position + getLightOffset(entity.get<Transform2D>().rotationDegrees, light.heightOffset);
        const Vector2i screenPosition =
            Vector2i(worldPosition.x - cameraPos.x, -1 * worldPosition.y + cameraPos.y) + Vector2i(WINDOW_WIDTH_GAME / 2, WINDOW_HEIGHT_GAME / 2);
        Color color = Color(light.color.r, light.color.b, light.color.g, light.color.a);

        // RESEARCH may want to add this as a param in the component
        constexpr f32 intensity = 1.0;
        s32 radius = light.radius;
        radius = ease(0, radius, intensity, Ease::OutQuad);

        Vector2 screenPosV(screenPosition.x, screenPosition.y);
        SetShaderValue(shader, mPositionUniform, &screenPosV, SHADER_UNIFORM_VEC2);

        Rectangle srcRect(0, 0, randomTexture.width, randomTexture.height);
        Rectangle dstRect(screenPosition.x - radius, screenPosition.y - radius, radius * 2, radius * 2);
        DrawTexturePro(randomTexture, srcRect, dstRect, Vector2(0, 0), 0, color);
    }

    EndMode2D();
    EndTextureMode();
}

void ShadowLightSystem::onEvent(ShaderReloadEvent) {
    mLightPosUniform = GetShaderLocation(ShaderManager::get(Shaders::ShadowLight), "lp1");
    mRadiusUniform = GetShaderLocation(ShaderManager::get(Shaders::ShadowLight), "radiusPixels");
}

void ShadowLightSystem::drawEntities() {
    // RESEARCH maybe pass angle/spread uniform?

    auto shader = ShaderManager::get(Shaders::ShadowLight);
    for (auto [entityid, entity] : getEntitiesMutable()) {
        ShaderManager::activate(Shaders::ShadowLight);

        const auto light = entity.get<ShadowLight>();
        const Vector2i entityPos = entity.get<Transform2D>().position;
        const Vector2f screenPos = worldToUVcoords(entityPos.as<f32>() + Vector2f(0, light.heightOffset));
        const Vector2 screenPosRL = Vector2(screenPos.x, screenPos.y);
        SetShaderValue(shader, mLightPosUniform, &screenPosRL, SHADER_UNIFORM_VEC2);

        const f32 lightRadiusPixels = light.radius;
        SetShaderValue(shader, mRadiusUniform, &lightRadiusPixels, SHADER_UNIFORM_FLOAT);
        const auto& tex = TextureManager::getRenderTexture(TextureID::DownscaledPostProcess).texture;
        DrawTextureRec(tex, Rectangle(0, 0, tex.width, -tex.height), Vector2(0, 0), light.color);
        EndShaderMode();
    }
}

}  // namespace whal
