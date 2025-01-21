#include "Block.h"

#include "Settings.h"
#include "Sys/System.h"

#include "Components/Collider.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "whalECS/src/ECS.h"

namespace whal {

ecs::Entity createBlock(Transform transform) {
    auto block = World.entity(false);
    if (!block.isValid()) {
        return block;
    }
    auto _ = ecs::DeferActivate(block);

    block.set(transform);
    block.add<DrawRect>();

    const s32 widthTileHL = PIXELS_PER_TILE / 2;
    const s32 heightTileHL = PIXELS_PER_TILE / 2;
    block.add(Collider(transform, Vector2i(widthTileHL, heightTileHL), CollisionLayer::Solid));

    return block;
}

ecs::Entity createBlock(Transform transform, DrawRect rect) {
    auto block = World.entity(false);
    if (!block.isValid()) {
        return block;
    }
    auto _ = ecs::DeferActivate(block);

    block.set(transform);
    block.add(rect);

    const s32 widthTileHL = PIXELS_PER_TILE / 2;
    const s32 heightTileHL = PIXELS_PER_TILE / 2;
    block.add(Collider(transform, Vector2i(widthTileHL, heightTileHL), CollisionLayer::Solid));

    return block;
}

ecs::Entity createBlock(Transform transform, Sprite sprite, WorldMaterial material) {
    auto block = World.entity(false);
    if (!block.isValid()) {
        return block;
    }
    auto _ = ecs::DeferActivate(block);

    block.set(transform);
    block.add(sprite);

    const s32 widthTileHL = PIXELS_PER_TILE / 2;
    const s32 heightTileHL = PIXELS_PER_TILE / 2;
    block.add(Collider(transform, Vector2i(widthTileHL, heightTileHL), CollisionLayer::Solid,
                       ColliderParams{
                           .material = material,
                       }));

    return block;
}

ecs::Entity createDecal(Transform transform, Sprite sprite, bool activate) {
    auto decal = World.entity(false);
    if (!decal.isValid()) {
        return decal;
    }

    decal.set(transform);
    decal.add(sprite);
    if (activate) {
        decal.activate();
    }
    return decal;
}

}  // namespace whal
