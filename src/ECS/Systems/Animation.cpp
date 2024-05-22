#include "Animation.h"

#include "ECS/Animator.h"
#include "ECS/Draw.h"
#include "Gfx/Texture.h"

namespace whal {

void AnimationSystem::update() {
    for (auto& [entityid, entity] : getEntitiesRef()) {
        auto& anim = entity.get<Animator>();
        if (anim.brain == nullptr) {
            continue;
        }
        auto& sprite = entity.get<Sprite>();

        if ((*anim.brain)(anim, entity)) {
            // frame changed
            const Frame frame = anim.getFrame();
            sprite.atlasPositionTexels = frame.atlasPositionTexels;
        }
    }
}

}  // namespace whal
