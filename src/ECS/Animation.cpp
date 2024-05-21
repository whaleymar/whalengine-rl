#include "Animation.h"

#include "ECS/Components/Animator.h"
#include "ECS/Components/Draw.h"
#include "ECS/Components/Transform.h"
#include "ECS/Lib/ECS.h"
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
            sprite.isVertsUpdateNeeded = true;
        } else {
            // frame unchanged
            auto& trans = entity.get<Transform>();
            if (trans.isDirectionChanged) {
                sprite.isVertsUpdateNeeded = true;
                trans.isDirectionChanged = false;
            }
        }
    }
}

}  // namespace whal
