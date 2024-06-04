#include "ECS/Systems/Gfx.h"

#include "Gfx/Texture.h"
#include "Settings.h"

#include "ECS/Systems/TagTrackers.h"
#include "Util/Print.h"
#include "Util/Vector.h"

#include "ECS/Draw.h"
#include "ECS/Transform.h"

namespace whal {

void SpriteSystem::onAdd(const ecs::Entity entity) {
    f32 fDepth = depthToFloat(entity.get<Sprite>().depth);
    auto prev = mSorted.before_begin();
    for (auto it = mSorted.begin(); it != mSorted.end(); ++it) {
        f32 fDepthNew = depthToFloat(it->get<Sprite>().depth);
        if (fDepthNew >= fDepth) {
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
    auto cameraPosF = toFloatVec(getCameraPosition());
    const Texture2D& spriteTexture = TextureManager::instance().getTextureAtlas(TEXNAME_SPRITE).getTexture();

    for (auto const& entity : mSorted) {
        Transform2D& trans = entity.get<Transform2D>();
        Sprite& sprite = entity.get<Sprite>();

        auto frameSize = toFloatVec(sprite.getFrameSizeTexels());
        f32 flipModifier = trans.facing == Facing::Left ? -1 : 1;
        Rectangle srcRect = Rectangle(sprite.atlasPositionTexels.x(), sprite.atlasPositionTexels.y(), flipModifier * frameSize.x(), frameSize.y());

        Vector2f dstSize = {frameSize.x() * sprite.scale.x() * FPIXELS_PER_TEXEL, frameSize.y() * sprite.scale.y() * FPIXELS_PER_TEXEL};

        // subtract size.y() so we draw from bottom left instead of top left
        // RESEARCH ? offset left by half a tile to fix camera detecting bottom left instead of bottom middle?
        Vector2f dstPosition = {trans.position.x() - cameraPosF.x(), -1 * trans.position.y() + cameraPosF.y() - dstSize.y()};

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

        DrawTexturePro(spriteTexture, srcRect, dstRect, {origin.x(), origin.y()}, trans.rotationDegrees, sprite.color);
    }
}

void DrawSystem::drawEntities() {
    auto cameraPosF = toFloatVec(getCameraPosition());

    // sorting not required since Draw components don't have transparency
    for (auto const& [entityid, entity] : getEntitiesRef()) {
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
    auto cameraPosF = toFloatVec(getCameraPosition());

    // sorting not required since Draw components don't have transparency
    for (auto const& [entityid, entity] : getEntitiesRef()) {
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

}  // namespace whal
