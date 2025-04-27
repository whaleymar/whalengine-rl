#include "SpriteOutlineSystem.h"

#include "Components/Draw.h"
#include "Components/Transform.h"

namespace whal {

void SpriteOutlineSystem::onAdd(ecs::Entity entity) {
    // to create an outline, we create a silhouette sprite that's offset by 1 pixel in each cardinal direction
    // this is (probably) more efficient than using a custom shader

    // const auto outlineDirs = {Direction::N, Direction::S, Direction::E, Direction::W, Direction::NE, Direction::NW, Direction::SE, Direction::SW};
    const auto outlineDirs = {Direction::N, Direction::S, Direction::E, Direction::W};
    const SpriteOutline& outlineCmp = entity.get<SpriteOutline>();
    Sprite silhouette = entity.get<Sprite>();
    silhouette.setFlag(Sprite::Silhouette);
    silhouette.color = outlineCmp.color;

    const Depth outlineDepth = entity.get<Transform>().depth;

    for (auto dir : outlineDirs) {
        ecs::Entity outline = entity.createChild("OutlineSprite", false);
        if (!outline.isValid()) {
            break;
        }
        outline.get<Transform>().depth = outlineDepth;
        outline.get<Transform>().translate(directionToVector(dir).as<f32>(), outline);
        outline.add(silhouette).add<IsOutline>().activate();
    }
}

static void syncOutlineSprite(ecs::Entity child, Sprite parentSprite) {
    if (!child.has<IsOutline>() || !child.has<Sprite>()) {
        return;
    }
    syncSpriteWithParent(child, parentSprite, true, true);
}

void syncSpriteWithParent(ecs::Entity child, Sprite parentSprite, bool syncShader, bool syncFlags) {
    if (!child.has<Sprite>()) {
        return;
    }
    Sprite& sprite = child.get<Sprite>();
    sprite.setFrame(parentSprite.getFrame());

    // sync custom shader stuff
    if (syncShader) {
        sprite.shader = parentSprite.shader;
        sprite.custom0b = parentSprite.custom0b;
    }
    if (syncFlags) {
        sprite.flags = parentSprite.flags | Sprite::Silhouette;
    }
}

void trySyncSpriteWithParent(ecs::Entity child) {
    Sprite* sprite = child.parent().tryGet<Sprite>();
    if (sprite) {
        syncSpriteWithParent(child, *sprite, false, false);
    }
}

void SpriteOutlineSystem::update() {
    for (const auto [entityid, entity] : getEntities()) {
        // sync the outline entity sprite frames and shader to the parent
        entity.forChild(&syncOutlineSprite, false, entity.get<Sprite>());
    }
}

}  // namespace whal
