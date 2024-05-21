#include "Block.h"

#include "Settings.h"

#include "ECS/Collision.h"
#include "ECS/Draw.h"
#include "ECS/Transform.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

Expected<ecs::Entity> createBlock(Transform2D transform) {
    auto& ecs = ecs::ECS::getInstance();

    auto expected = ecs.entity();
    if (!expected.isExpected()) {
        return expected;
    }
    auto block = expected.value();

    block.add(transform);
    block.add<Draw>();

    const s32 widthTileHL = PIXELS_PER_TEXEL * TEXELS_PER_TILE / 2;
    const s32 heightTileHL = PIXELS_PER_TEXEL * TEXELS_PER_TILE / 2;
    block.add(SolidCollider(transform, Vector2i(widthTileHL, heightTileHL)));

    return block;
}

Expected<ecs::Entity> createBlock(Transform2D transform, Draw draw) {
    auto& ecs = ecs::ECS::getInstance();

    auto expected = ecs.entity();
    if (!expected.isExpected()) {
        return expected;
    }
    auto block = expected.value();

    block.add(transform);
    block.add(draw);

    const s32 widthTileHL = PIXELS_PER_TEXEL * TEXELS_PER_TILE / 2;
    const s32 heightTileHL = PIXELS_PER_TEXEL * TEXELS_PER_TILE / 2;
    block.add(SolidCollider(transform, Vector2i(widthTileHL, heightTileHL)));

    return block;
}

Expected<ecs::Entity> createBlock(Transform2D transform, Sprite sprite, Material material) {
    auto& ecs = ecs::ECS::getInstance();

    auto expected = ecs.entity();
    if (!expected.isExpected()) {
        return expected;
    }
    auto block = expected.value();

    block.add(transform);
    block.add(sprite);

    const s32 widthTileHL = PIXELS_PER_TEXEL * TEXELS_PER_TILE / 2;
    const s32 heightTileHL = PIXELS_PER_TEXEL * TEXELS_PER_TILE / 2;
    block.add(SolidCollider(transform, Vector2i(widthTileHL, heightTileHL), material));

    return block;
}

Expected<ecs::Entity> createDecal(Transform2D transform, Sprite sprite) {
    auto& ecs = ecs::ECS::getInstance();

    auto expected = ecs.entity();
    if (!expected.isExpected()) {
        return expected;
    }
    auto decal = expected.value();

    decal.add(transform);
    decal.add(sprite);
    return decal;
}

}  // namespace whal
