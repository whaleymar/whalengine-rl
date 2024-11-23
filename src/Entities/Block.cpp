#include "Block.h"

#include "Settings.h"
#include "Sys/System.h"

#include "Components/Collision.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "whalECS/src/ECS.h"

namespace whal {

Expected<ecs::Entity> createBlock(Transform transform) {
    auto expected = World.entity(false);
    if (!expected.isExpected()) {
        return expected;
    }
    auto _ = ecs::DeferActivate(expected.value());
    auto block = expected.value();

    block.add(transform);
    block.add(DrawRect());

    const s32 widthTileHL = PIXELS_PER_TILE / 2;
    const s32 heightTileHL = PIXELS_PER_TILE / 2;
    block.add(Collider::Solid(transform, Vector2i(widthTileHL, heightTileHL)));

    return block;
}

Expected<ecs::Entity> createBlock(Transform transform, DrawRect rect) {
    auto expected = World.entity(false);
    if (!expected.isExpected()) {
        return expected;
    }
    auto _ = ecs::DeferActivate(expected.value());
    auto block = expected.value();

    block.add(transform);
    block.add(rect);

    const s32 widthTileHL = PIXELS_PER_TILE / 2;
    const s32 heightTileHL = PIXELS_PER_TILE / 2;
    block.add(Collider::Solid(transform, Vector2i(widthTileHL, heightTileHL)));

    return block;
}

Expected<ecs::Entity> createBlock(Transform transform, Sprite sprite, WorldMaterial material) {
    auto expected = World.entity(false);
    if (!expected.isExpected()) {
        return expected;
    }
    auto _ = ecs::DeferActivate(expected.value());
    auto block = expected.value();

    block.add(transform);
    block.add(sprite);

    const s32 widthTileHL = PIXELS_PER_TILE / 2;
    const s32 heightTileHL = PIXELS_PER_TILE / 2;
    block.add(Collider::Solid(transform, Vector2i(widthTileHL, heightTileHL), material));

    return block;
}

Expected<ecs::Entity> createDecal(Transform transform, Sprite sprite, bool activate) {
    auto expected = World.entity(false);
    if (!expected.isExpected()) {
        return expected;
    }
    auto decal = expected.value();

    decal.add(transform);
    decal.add(sprite);
    if (activate) {
        decal.activate();
    }
    return decal;
}

}  // namespace whal
