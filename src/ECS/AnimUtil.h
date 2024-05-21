#pragma once

#include <format>
#include <vector>

#include "ECS/Animator.h"
#include "Gfx/Texture.h"
#include "Util/Print.h"

namespace whal {

using AnimInfo = std::vector<std::tuple<const char*, s32, s32, f32>>;

inline void loadAnimations(Animator& animator, const AnimInfo& animInfo) {
    auto& spriteTexture = TextureManager::instance().getTextureAtlas(TEXNAME_SPRITE);

    for (auto [animBaseName, id, count, secsPerFrame] : animInfo) {
        std::vector<Frame> frames;
        for (s32 i = 0; i < count; i++) {
            auto animName = std::format("{}{}", animBaseName, i + 1);

            auto frame = spriteTexture.getFrame(animName.c_str());
            if (frame) {
                frames.push_back(*frame);
            } else {
                print("Failed to load animation frame: ", animName);
            }
        }

        animator.animations.push_back(Animation(id, frames, secsPerFrame));
    }
    animator.resetAnimation();
}

}  // namespace whal
