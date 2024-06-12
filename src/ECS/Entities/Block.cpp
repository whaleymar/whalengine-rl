#include "Block.h"

#include "Settings.h"
#include "Systems/System.h"

#include "ECS/Collision.h"
#include "ECS/Draw.h"
#include "ECS/Transform.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

Expected<ecs::Entity> createBlock(Transform2D transform) {
    auto expected = System::ecs->entity(false);
    if (!expected.isExpected()) {
        return expected;
    }
    auto _ = ecs::DeferActivate(expected.value());
    auto block = expected.value();

    block.add(transform);
    block.add<Draw>();

    const s32 widthTileHL = PIXELS_PER_TEXEL * TEXELS_PER_TILE / 2;
    const s32 heightTileHL = PIXELS_PER_TEXEL * TEXELS_PER_TILE / 2;
    block.add(Collider::Solid(transform, Vector2i(widthTileHL, heightTileHL)));

    return block;
}

Expected<ecs::Entity> createBlock(Transform2D transform, Draw draw) {
    auto expected = System::ecs->entity(false);
    if (!expected.isExpected()) {
        return expected;
    }
    auto _ = ecs::DeferActivate(expected.value());
    auto block = expected.value();

    block.add(transform);
    block.add(draw);

    const s32 widthTileHL = PIXELS_PER_TEXEL * TEXELS_PER_TILE / 2;
    const s32 heightTileHL = PIXELS_PER_TEXEL * TEXELS_PER_TILE / 2;
    block.add(Collider::Solid(transform, Vector2i(widthTileHL, heightTileHL)));

    return block;
}

Expected<ecs::Entity> createBlock(Transform2D transform, Sprite sprite, WorldMaterial material) {
    auto expected = System::ecs->entity(false);
    if (!expected.isExpected()) {
        return expected;
    }
    auto _ = ecs::DeferActivate(expected.value());
    auto block = expected.value();

    block.add(transform);
    block.add(sprite);

    const s32 widthTileHL = PIXELS_PER_TEXEL * TEXELS_PER_TILE / 2;
    const s32 heightTileHL = PIXELS_PER_TEXEL * TEXELS_PER_TILE / 2;
    block.add(Collider::Solid(transform, Vector2i(widthTileHL, heightTileHL), material));

    return block;
}

Expected<ecs::Entity> createDecal(Transform2D transform, Sprite sprite) {
    auto expected = System::ecs->entity(false);
    if (!expected.isExpected()) {
        return expected;
    }
    auto _ = ecs::DeferActivate(expected.value());
    auto decal = expected.value();

    decal.add(transform);
    decal.add(sprite);
    return decal;
}

}  // namespace whal
