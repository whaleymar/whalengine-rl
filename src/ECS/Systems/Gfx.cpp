#include "ECS/Systems/Gfx.h"
#include <raylib.h>

#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"
#include "Settings.h"

#include "ECS/Systems/TagTrackers.h"
#include "Util/Vector.h"

#include "ECS/Draw.h"
#include "ECS/Transform.h"

namespace whal {

void SpriteSystem::onAdd(const ecs::Entity entity) {
    // insert entities into sorted order, based on depth, then shader
    f32 fDepth = depthToFloat(entity.get<Sprite>().depth);
    s16 shaderIx = static_cast<s16>(entity.get<Sprite>().shader);

    auto prev = mSorted.before_begin();
    for (auto it = mSorted.begin(); it != mSorted.end(); ++it) {
        Sprite sprite = it->get<Sprite>();
        f32 curDepth = depthToFloat(sprite.depth);
        s16 curShaderIx = static_cast<s16>(sprite.shader);
        if (curDepth > fDepth || (curDepth == fDepth && curShaderIx >= shaderIx)) {
            mSorted.insert_after(prev, entity);
            return;
        }
        prev = it;
    }
    mSorted.insert_after(prev, entity);
}

void SpriteSystem::onRemove(const ecs::Entity entity) {
    mSorted.remove_if([entity](const ecs::Entity& e) { return e.id() == entity.id(); });
}

void SpriteSystem::drawEntities() {
    if (mSorted.empty()) {
        return;
    }

    auto cameraPosF = getCameraPositionPrecise();
    // auto cameraPosF = toFloatVec(getCameraPosition());

    const Texture2D& spriteTexture = TextureManager::instance().getTextureAtlas(TEXNAME_SPRITE).getTexture();

    Shaders prevShader = mSorted.begin()->get<Sprite>().shader;
    BeginShaderMode(ShaderManager::get(prevShader));
    for (auto const entity : mSorted) {
        Shaders newShader = entity.get<Sprite>().shader;
        if (newShader != prevShader) {
            prevShader = newShader;
            EndShaderMode();
            BeginShaderMode(ShaderManager::get(newShader));
        }
        drawEntity(entity, spriteTexture, cameraPosF);
    }
    EndShaderMode();
}

void SpriteSystem::drawEntity(ecs::Entity entity, const Texture2D& spriteTexture, const Vector2f cameraPosF) {
    Transform2D& trans = entity.get<Transform2D>();
    Vector2f posF = toFloatVec(trans.position);
    Sprite& sprite = entity.get<Sprite>();

    const Vector2i frameSize = sprite.getFrameSizeTexels();
    const s32 flipModifier = trans.facing == Facing::Left ? -1 : 1;
    const Rectangle srcRect = Rectangle(sprite.atlasPositionTexels.x(), sprite.atlasPositionTexels.y(), flipModifier * frameSize.x(), frameSize.y());

    Vector2f dstSize = {frameSize.x() * sprite.scale.x() * FPIXELS_PER_TEXEL, frameSize.y() * sprite.scale.y() * FPIXELS_PER_TEXEL};

    // subtract size.y() so we draw from bottom left instead of top left
    Vector2f dstPosition = {posF.x() - cameraPosF.x(), -1.0f * posF.y() + cameraPosF.y() - dstSize.y()};

    Vector2f origin;
    if (trans.rotationDegrees == 0) {
        // I want to scale from the bottom middle, but draw from the bottom left, so move dstPosition right by halfx
        // TODO this won't work for scaling from the middle though
        origin = Vector2f(dstSize.x() * 0.5, 0);
    } else {
        // TODO y position not centered for all collider sizes
        // this is because i place colliders based on transform (bottom), so the projectile's bottom always passes through mouse click location
        // so I think this code is fine, but i need to change the physics system to draw centered colliders based on some param

        // draw centered
        // dstPosition += dstSize * Vector2f(0, 0.75);
        dstPosition += dstSize * Vector2f(0, 0.5);
        // dstPosition += dstSize * Vector2f(0, 0.25);
        origin = dstSize * 0.5;
    }

    Rectangle dstRect = Rectangle(dstPosition.x(), dstPosition.y(), dstSize.x(), dstSize.y());

    // TODO should sort entities so I only change shaders 2 * number of depth values times
    // if (entity.has<Silhouette>()) {
    //     BeginShaderMode(ShaderManager::get(Shaders::Silhouette));
    //     DrawTexturePro(spriteTexture, srcRect, dstRect, {origin.x(), origin.y()}, trans.rotationDegrees, sprite.color);
    //     EndShaderMode();
    // } else {
    DrawTexturePro(spriteTexture, srcRect, dstRect, {origin.x(), origin.y()}, trans.rotationDegrees, sprite.color);
    // }
}

void DrawSystem::drawEntities() {
    auto cameraPosF = getCameraPositionPrecise();
    // auto cameraPosF = toFloatVec(getCameraPosition());

    // sorting not required since Draw components don't have transparency
    for (auto const [entityid, entity] : getEntitiesRef()) {
        Transform2D& trans = entity.get<Transform2D>();
        Draw& draw = entity.get<Draw>();

        auto frameSize = toFloatVec(draw.getFrameSizeTexels());
        Vector2f dstSize = {frameSize.x() * draw.scale.x() * FPIXELS_PER_TEXEL, frameSize.y() * draw.scale.y() * FPIXELS_PER_TEXEL};

        // subtract size.y() so we draw from bottom left instead of top left
        Vector2f dstPosition = {trans.position.x() - cameraPosF.x(), -1 * trans.position.y() + cameraPosF.y() - dstSize.y()};
        // add halfX to pos to match the origin thingy done w/ sprites
        dstPosition -= {dstSize.x() * 0.5f, 0};
        Rectangle dstRect = Rectangle(dstPosition.x(), dstPosition.y(), dstSize.x(), dstSize.y());
        DrawRectangleRec(dstRect, draw.color);
    }
}

void DrawDebugSystem::drawEntities() {
    auto cameraPosF = getCameraPositionPrecise();
    // auto cameraPosF = toFloatVec(getCameraPosition());

    // sorting not required since Draw components don't have transparency
    for (auto const [entityid, entity] : getEntitiesRef()) {
        Transform2D& trans = entity.get<Transform2D>();
        Draw& draw = entity.get<DrawDebug>();

        auto frameSize = toFloatVec(draw.getFrameSizeTexels());
        Vector2f dstSize = {frameSize.x() * draw.scale.x() * FPIXELS_PER_TEXEL, frameSize.y() * draw.scale.y() * FPIXELS_PER_TEXEL};

        // subtract size.y() so we draw from bottom left instead of top left
        Vector2f dstPosition = {trans.position.x() - cameraPosF.x(), -1 * trans.position.y() + cameraPosF.y() - dstSize.y()};
        // add halfX to pos to match the origin thingy done w/ sprites
        dstPosition -= {dstSize.x() * 0.5f, 0};
        Rectangle dstRect = Rectangle(dstPosition.x(), dstPosition.y(), dstSize.x(), dstSize.y());
        DrawRectangleRec(dstRect, draw.color);
    }
}

void FadeOutSystem::update() {
    f32 dt = System::dt();
    for (auto [entityid, entity] : getEntitiesCopy()) {
        auto& fadeOutComponent = entity.get<FadeOut>();

        fadeOutComponent.secondsRemaining -= dt;
        u8 alpha = fadeOutComponent.getAlpha();

        if (entity.has<Sprite>()) {
            entity.get<Sprite>().setAlpha(alpha);
        } else if (entity.has<Draw>()) {
            entity.get<Draw>().setAlpha(alpha);
        }

        if (fadeOutComponent.isDone()) {
            entity.remove<FadeOut>();
        }
    }
}

}  // namespace whal
