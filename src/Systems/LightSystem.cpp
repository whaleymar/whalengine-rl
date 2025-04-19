#include "LightSystem.h"

#include <cmath>
#include <raylib.h>
#include <rlgl.h>

#include "Components/Light.h"
#include "Components/Transform.h"
#include "Events/Events.h"
#include "Gfx/Color.h"
#include "Gfx/Coordinates.h"
#include "Gfx/RaylibUtil.h"
#include "Gfx/Shader.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Settings.h"
#include "Sys/Renderer.h"
#include "Sys/System.h"

#include "Util/Vector.h"

namespace whal {

void PointLightSystem::draw(const gfx::RenderContext& ctx) const {
    Shader& shader = ShaderMgr::get("PointLight");
    shader.bind();

    // const auto depthTex = TextureManager::getRenderTexture(TextureID::OcclusionDepth).texture;
    const auto colorTex = TextureManager::getRenderTexture(TextureID::OcclusionColor).texture;
    for (auto [entityid, entity] : getEntities()) {
        const PointLight& light = entity.get<PointLight>();
        const Transform& trans = entity.get<Transform>();
        const Vector2i worldPosition = trans.apply(Vector2i(0, light.heightOffset));
        const Vector2i screenPosition = Vector2i(worldPosition.x, -worldPosition.y);
        Color color = light.color;
        s32 radius = light.radius;

        const rl::Rectangle srcRect(0, 0, colorTex.width, colorTex.height);
        const rl::Rectangle dstRect(screenPosition.x - radius, screenPosition.y - radius, radius * 2, radius * 2);
        gfx::DrawSpriteHDR(colorTex, srcRect, dstRect, rl::Vector2(0, 0), 0, color.asRL(), rl::Vector3{}, screenPosition.x, screenPosition.y);
    }
    shader.unbind();
}

void BoxLightSystem::draw(const gfx::RenderContext& ctx) const {
    Shader& shader = ShaderMgr::get("AabbLight");
    shader.bind();

    const auto randomTexture = Graphics.getTemporaryRT(WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME);
    for (auto [entityid, entity] : getEntities()) {
        const BoxLight& light = entity.get<BoxLight>();
        const Transform& trans = entity.get<Transform>();
        const Vector2i worldPosition = trans.apply(light.offset);
        Vector2i drawPosition = Vector2i(worldPosition.x, -worldPosition.y);
        Vector2i screenPosition = worldToScreenCoords(worldPosition.as<f32>(), ctx.cameraPosition, ScreenResolution::Game);
        Color color = light.color;
        s32 radius = light.radius;

        const Vector2i lightBounds(radius + light.halfLen.x, radius + light.halfLen.y);
        const Vector2i destPosition = drawPosition - lightBounds;
        const Vector2i destSize = lightBounds * 2;

        const rl::Rectangle srcRect(0, 0, light.halfLen.x * 2, light.halfLen.y * 2);
        const rl::Rectangle dstRect(destPosition.x, destPosition.y, destSize.x, destSize.y);

        gfx::DrawSpriteHDR(randomTexture.texture, srcRect, dstRect, rl::Vector2(0, 0), 0, color.asRL(),
                           rl::Vector3(screenPosition.x, screenPosition.y, light.radius));
    }
    shader.unbind();
    Graphics.releaseTemporaryRT(randomTexture);
}

void ShadowLightSystem::draw(const gfx::RenderContext& ctx) const {
    // RESEARCH light angle/spread values?

    Shader& shader = ShaderMgr::get("ShadowLight");
    const auto depthTex = TextureManager::getRenderTexture(TextureID::OcclusionDepth).texture;
    const auto allDepthTex = TextureManager::getRenderTexture(TextureID::Depth).texture;
    const auto distanceFieldTex = TextureManager::getRenderTexture(TextureID::DistanceField).texture;
    const auto colorTex = TextureManager::getRenderTexture(TextureID::OcclusionColor);

    // whatever convoluted shit I'm doing, I can't make it work with the camera transform...
    rl::EndMode2D();

    // must match what's in spritefrag.glsl
    const f32 depthScalar = 20.0f;

    shader.setVector2("_DistanceFieldSize", Vector2f(distanceFieldTex.width, distanceFieldTex.height));
    shader.setTexture("depthBuf", depthTex);
    shader.setTexture("_DistanceField", distanceFieldTex);
    shader.setTexture("_AllDepth", allDepthTex);
    shader.bind();
    for (auto [entityid, entity] : getEntities()) {
        const ShadowLight& light = entity.get<ShadowLight>();
        const Transform& trans = entity.get<Transform>();

        Vector2f worldPosition = trans.apply(Vector2f(0, light.heightOffset));
        const Vector2f screenPos = worldToUVcoords(worldPosition).as<f32>();
        const f32 lightDepth = static_cast<f32>(trans.depth) / 255.0f * depthScalar;

        rl::Texture tex = colorTex.texture;
        gfx::DrawSpriteHDR(tex, rl::Rectangle(0, 0, tex.width, -tex.height), rl::Rectangle(0, 0, tex.width, tex.height), rl::Vector2(0, 0), 0.0f,
                           light.color.asRL(), rl::Vector3(screenPos.x, screenPos.y, light.radius), lightDepth);
    }

    shader.unbind();
    rl::BeginMode2D(ctx.camera);
}

}  // namespace whal
