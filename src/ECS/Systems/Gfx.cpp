#include "ECS/Systems/Gfx.h"

#include "Gfx/Texture.h"
#include "Settings.h"

#include "ECS/Systems/TagTrackers.h"
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
        Rectangle srcRect = Rectangle(sprite.atlasPositionTexels.x(), sprite.atlasPositionTexels.y(), frameSize.x(), frameSize.y());

        Vector2f dstSize = {frameSize.x() * sprite.scale.x() * FPIXELS_PER_TEXEL, frameSize.y() * sprite.scale.y() * FPIXELS_PER_TEXEL};
        Vector2f dstPosition = {trans.position.x() - cameraPosF.x(), trans.position.y() - cameraPosF.y()};
        Rectangle dstRect = Rectangle(dstPosition.x(), dstPosition.y(), dstSize.x(), dstSize.y());

        Vector2f origin = dstSize * 0.5;

        DrawTexturePro(spriteTexture, srcRect, dstRect, {origin.x(), origin.y()}, 0.0f, sprite.color);
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
        Vector2f dstPosition = {trans.position.x() - cameraPosF.x(), trans.position.y() - cameraPosF.y()};
        Rectangle dstRect = Rectangle(dstPosition.x(), dstPosition.y(), dstSize.x(), dstSize.y());
        DrawRectangleRec(dstRect, draw.color);
    }
}

}  // namespace whal
