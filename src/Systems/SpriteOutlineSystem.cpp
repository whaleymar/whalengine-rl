#include "SpriteOutlineSystem.h"

#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Util/String.h"

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
        ecs::Entity outline = entity.createChild("OutlineSprite");
        if (!outline.isValid()) {
            break;
        }
        outline.get<Transform>().depth = outlineDepth;
        outline.get<Transform>().translate(directionToVector(dir).as<f32>(), outline);
        outline.add(silhouette);
    }
}

void sync(ecs::Entity child, Sprite parentSprite) {
    if (!isEqualString(child.name(), "OutlineSprite") || !child.has<Sprite>()) {
        return;
    }
    Sprite& sprite = child.get<Sprite>();
    sprite.setFrame(parentSprite.getFrame());

    // sync custom shader stuff
    sprite.shader = parentSprite.shader;
    sprite.flags = parentSprite.flags | Sprite::Silhouette;
    sprite.custom0b = parentSprite.custom0b;
}

void SpriteOutlineSystem::update() {
    for (const auto [entityid, entity] : getEntities()) {
        // sync the outline entity sprite frames and shader to the parent
        entity.forChild(&sync, false, entity.get<Sprite>());
    }
}

}  // namespace whal
