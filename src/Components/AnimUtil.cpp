#include "AnimUtil.h"

#include <cassert>

#include "Components/Animator.h"
#include "Gfx/Texture.h"
#include "Util/Print.h"

namespace whal {

void loadAnimations(Animator& animator, const AnimInfo& animInfo) {
    const auto& spriteTexture = TextureManager::getAtlas(TEXNAME_SPRITE);

    for (auto [animBaseName, id, count, secsPerFrame] : animInfo) {
        std::vector<Frame> frames;
        for (s32 i = 0; i < count; i++) {
            auto animName = whal_format("{}{}", animBaseName, i + 1);

            auto frame = spriteTexture.getFrame(animName.c_str());
#ifndef NDEBUG
            if (!frame) {
                print("Failed to load animation frame:", animName);
                assert(false);
            }
#endif
            frames.push_back(*frame);
        }

        animator.animations.push_back(Animation(id, frames, secsPerFrame));
    }
    animator.resetAnimation();
}

}  // namespace whal
