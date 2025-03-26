#include "AnimationSystem.h"

#include "Components/Animator.h"
#include "Components/Draw.h"

namespace whal {

void AnimationSystem::update() {
    for (auto [entityid, entity] : getEntities()) {
        auto& anim = entity.get<Animator>();
        anim._isAnimationFinishedThisFrame = false;  // reset flag

        // ensure the entity has a sprite
        if (!entity.has<Sprite>()) {
            entity.add<Sprite>();
        }

        if (anim.brain == nullptr) {
            continue;
        }

        Sprite& sprite = entity.get<Sprite>();
        if ((*anim.brain)(anim, entity)) {
            // frame changed
            const Frame frame = anim.getFrame();
            sprite.atlasPosition = frame.atlasPosition.as<f32>();
            sprite.frameSize = frame.size.as<f32>();
        }
    }
}

}  // namespace whal
