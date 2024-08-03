#pragma once

#include <vector>

#include "Gfx/Texture.h"
#include "Util/Types.h"

namespace whal {

namespace ecs {
class Entity;
}

struct Animator;
struct Animation;

// returns True if frame changed
using AnimBrain = bool (*)(Animator& animator, ecs::Entity entity);

bool basicAnimation(Animator& animator, ecs::Entity entity);
bool basicAnimationUnsquish(Animator& animator, ecs::Entity entity);

/*
 * the animator controls which animation an entity is using and handles frame-advancing
 * it contains a list of animations + a "brain" function which changes animations and advances frames
 *
 * constraint: all animations have the same dimensions
 */

using AnimInfo = std::vector<std::tuple<const char*, s32, s32, f32>>;
struct Animator {
    Animator() = default;
    Animator(AnimInfo animInfo, bool isLooping_ = true);
    Animator(AnimInfo animInfo, AnimBrain brain_, bool isLooping_ = true);

    std::vector<Animation> animations;
    AnimBrain brain = &basicAnimation;
    s32 curAnimIx = 0;
    f32 curAnimDuration = 0.0;
    s32 curFrameIx = 0;
    f32 curFrameDuration = 0.0;
    bool isLooping = true;

    Frame getFrame() const;
    Animation& getAnimation();
    bool setAnimation(s32 id);
    void nextFrame();
    void resetAnimation();
    void setLooping(bool loop);
};

// an animation is a sequence of same-sized frames
struct Animation {
    Animation();
    Animation(s32 id, std::vector<Frame> frames, f32 secondsPerFrame = 0.25);

    Frame getFrame(s32 ix) const;
    s32 getFrameCount() const;

    std::vector<Frame> frames;
    s32 id;
    f32 secondsPerFrame = 0.25;
};

}  // namespace whal
