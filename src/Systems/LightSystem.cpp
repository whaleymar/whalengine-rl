#include "LightSystem.h"

#include <cmath>
#include <raylib.h>
#include <rlgl.h>

#include "Components/Light.h"
#include "Components/Tags.h"
#include "Components/Transform.h"

#include "Events/Events.h"
#include "Gfx/Color.h"
#include "Gfx/Coordinates.h"
#include "Gfx/RaylibUtil.h"
#include "Gfx/Shader.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Settings.h"
#include "Sys/System.h"
#include "Systems/TagSystems.h"

#include "Util/Easing.h"
#include "Util/Vector.h"

namespace whal {

const Color COLOR_AMBIENT = Colors::Black;

// RESEARCH may want to put intensity as a param in each light component

void PointLightSystem::draw(const gfx::RenderContext& ctx) const {
    Shader& shader = ShaderMgr::get("PointLight");
    shader.bind();

    // const auto depthTex = TextureManager::getRenderTexture(TextureID::OcclusionDepth).texture;
    const auto colorTex = TextureManager::getRenderTexture(TextureID::OcclusionColor).texture;
    for (auto [entityid, entity] : getEntities()) {
        if (entity.has<Invisible>()) {
            continue;
        }
        PointLight light = entity.get<PointLight>();
        const auto trans = entity.get<Transform>();
        const Vector2i worldPosition = trans.apply(Vector2i(0, light.heightOffset));
        const Vector2i screenPosition = Vector2i(worldPosition.x, -worldPosition.y);
        Color color = light.color;

        constexpr f32 intensity = 1.0;
        s32 radius = light.radius;

        color.a = std::lerp(COLOR_AMBIENT.a, color.a, intensity);
        radius = ease(radius / 2, radius, intensity, Ease::InQuad);

        shader.setVector2("position", screenPosition.asRL());
        // shader.setTexture("occlusionDepthTex", depthTex);
        // shader.setFloat("lightDepth", depthToFloat(trans.depth));
        Graphics.setUniforms(shader.get());

        const rl::Rectangle srcRect(0, 0, colorTex.width, colorTex.height);
        const rl::Rectangle dstRect(screenPosition.x - radius, screenPosition.y - radius, radius * 2, radius * 2);
        gfx::DrawSpriteHDR(colorTex, srcRect, dstRect, rl::Vector2(0, 0), 0, color.asRL());
    }
    shader.unbind();
}

void BoxLightSystem::draw(const gfx::RenderContext& ctx) const {
    Shader& shader = ShaderMgr::get("AabbLight");

    const auto randomTexture = Graphics.getTemporaryRT(WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME);
    for (auto [entityid, entity] : getEntities()) {
        if (entity.has<Invisible>()) {
            continue;
        }

        BoxLight light = entity.get<BoxLight>();
        const auto trans = entity.get<Transform>();
        const Vector2i worldPosition = trans.apply(light.offset);
        Vector2i drawPosition = Vector2i(worldPosition.x, -worldPosition.y);
        Vector2i screenPosition = worldToScreenCoords(worldPosition.as<f32>(), ctx.cameraPosition, ScreenResolution::Game);
        Color color = light.color;

        constexpr f32 intensity = 1.0;
        s32 radius = light.radius;

        // apply fading
        color = Color::lerp(COLOR_AMBIENT, color, intensity);

        // light falls off quadratically
        radius = ease(radius / 2, radius, intensity, Ease::InQuad);

        shader.setVector2("lightpos", screenPosition.asRL());
        shader.setVector2("lighthalflen", light.halfLen.asRL());
        shader.setFloat("lightradius", light.radius);

        // shader.setFloat("lightDepth", depthToFloat(trans.depth));
        // shader.setTexture("occlusionDepthTex", depthTex);

        // I have to bind/unbind the shader for every light, otherwise the uniforms from one light will affect the others.
        // This can be solved by instancing

        const Vector2i lightBounds(radius + light.halfLen.x, radius + light.halfLen.y);
        const Vector2i destPosition = drawPosition - lightBounds;
        const Vector2i destSize = lightBounds * 2;

        const rl::Rectangle srcRect(0, 0, randomTexture.texture.width, randomTexture.texture.height);
        const rl::Rectangle dstRect(destPosition.x, destPosition.y, destSize.x, destSize.y);

        shader.bind();
        gfx::DrawSpriteHDR(randomTexture.texture, srcRect, dstRect, rl::Vector2(0, 0), 0, color.asRL());
        shader.unbind();
    }
    Graphics.releaseTemporaryRT(randomTexture);
}

void ShadowLightSystem::draw(const gfx::RenderContext& ctx) const {
    // RESEARCH maybe pass angle/spread uniform?
    // RESEARCH instead of binding new uniforms for every draw call, it would make more sense to pass an array of uniforms to the shader once

    Shader& shader = ShaderMgr::get("ShadowLight");
    const auto depthTex = TextureManager::getRenderTexture(TextureID::OcclusionDepth).texture;
    const auto allDepthTex = TextureManager::getRenderTexture(TextureID::Depth).texture;
    const auto distanceFieldTex = TextureManager::getRenderTexture(TextureID::DistanceField).texture;
    const auto colorTex = TextureManager::getRenderTexture(TextureID::OcclusionColor);

    // whatever convoluted shit I'm doing, I can't make it work with the camera transform...
    rl::EndMode2D();

    // must match what's in spritefrag.glsl
    const f32 depthScalar = 20.0f;
    for (auto [entityid, entity] : getEntities()) {
        const auto light = entity.get<ShadowLight>();
        const auto trans = entity.get<Transform>();

        const Vector2f screenPos = worldToUVcoords((trans.positionPx + Vector2i(0, light.heightOffset)).as<f32>());
        const f32 lightDepth = static_cast<f32>(trans.depth) / 255.0f * depthScalar;

        // Set shader values
        shader.setVector2("lp1", screenPos);
        shader.setFloat("radiusPixels", light.radius);
        shader.setFloat("lightDepth", lightDepth);

        // RESEARCH feel like i should be able to just set these once...
        shader.setVector2("_DistanceFieldSize", Vector2f(distanceFieldTex.width, distanceFieldTex.height));
        shader.setTexture("depthBuf", depthTex);
        shader.setTexture("_DistanceField", distanceFieldTex);
        shader.setTexture("_AllDepth", allDepthTex);

        shader.bind();
        gfx::DrawRenderTextureHDR(colorTex, light.color);
        shader.unbind();
    }

    rl::BeginMode2D(ctx.camera);
}

}  // namespace whal
