#include "Animation.h"

#include "Components/Animator.h"
#include "Components/Draw.h"
#include "Gfx/Texture.h"

namespace whal {

void AnimationSystem::update() {
    for (auto& [entityid, entity] : getEntitiesMutable()) {
        auto& anim = entity.get<Animator>();
        if (anim.brain == nullptr) {
            continue;
        }
        auto& sprite = entity.get<Draw>().getSprite();

        if ((*anim.brain)(anim, entity)) {
            // frame changed
            const Frame frame = anim.getFrame();
            sprite.atlasPosition = frame.atlasPosition;
        }
    }
}

}  // namespace whal
