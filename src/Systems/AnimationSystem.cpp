#include "AnimationSystem.h"

#include "Components/Animator.h"
#include "Components/Draw.h"

namespace whal {

void AnimationSystem::update() {
    for (auto& [entityid, entity] : getEntitiesMutable()) {
        auto& anim = entity.get<Animator>();
        if (anim.brain == nullptr) {
            continue;
        }
        auto& sprite = entity.get<Sprite>();

        if ((*anim.brain)(anim, entity)) {
            // frame changed
            const Frame frame = anim.getFrame();
            sprite.atlasPosition = frame.atlasPosition;
        }
    }
}

}  // namespace whal
