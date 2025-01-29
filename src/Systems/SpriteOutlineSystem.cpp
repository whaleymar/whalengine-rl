#include "SpriteOutlineSystem.h"

#include "Components/Draw.h"
#include "Components/Transform.h"

namespace whal {

struct IsOutlineSprite {};

void SpriteOutlineSystem::onAdd(ecs::Entity entity) {
    // to create an outline, we create a silhouette sprite that's offset by 1 pixel in each cardinal direction
    // this is (probably) more efficient than using a custom shader

    const auto outlineDirs = {Direction::N, Direction::S, Direction::E, Direction::W, Direction::NE, Direction::NW, Direction::SE, Direction::SW};
    const SpriteOutline& outlineCmp = entity.get<SpriteOutline>();
    Sprite silhouette = entity.get<Sprite>();
    silhouette.setFlag(Sprite::Silhouette);
    silhouette.color = outlineCmp.color;

    // outline depth is one layer below the parent entity
    const Depth outlineDepth = static_cast<Depth>(static_cast<u8>(entity.get<Transform>().depth) - 1);

    for (auto dir : outlineDirs) {
        ecs::Entity outline = entity.createChild();
        if (!outline.isValid()) {
            break;
        }
        outline.get<Transform>().depth = outlineDepth;
        outline.get<Transform>().translate(directionToVector(dir).as<f32>(), outline);
        outline.add(silhouette);
        outline.add<IsOutlineSprite>();
    }
}

void sync(ecs::Entity child) {
    if (!child.has<IsOutlineSprite>() || !child.has<Sprite>()) {
        return;
    }
    child.get<Sprite>().setFrame(child.parent().get<Sprite>().getFrame());
}

void SpriteOutlineSystem::update() {
    for (const auto [entityid, entity] : getEntities()) {
        // sync the outline entity sprite frames to the parent
        entity.forChild(&sync, false);
    }
}

}  // namespace whal
