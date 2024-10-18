#include "Animator.h"

#include <cstring>

#include "Components/AnimUtil.h"
#include "Components/Draw.h"
#include "whalECS/src/ECS.h"

#include "Sys/System.h"
#include "Util/Print.h"

namespace whal {

bool basicAnimation(Animator& animator, ecs::Entity entity) {
    const Animation& anim = animator.getAnimation();

    // play animations at normal speed if paused? Seems cute
    f32 dt = System::isPaused() ? System::time.getUnmodified() : System::dt();
    animator.curFrameDuration += dt;
    animator.curAnimDuration += dt;
    if (animator.curFrameDuration >= anim.secondsPerFrame) {
        animator.nextFrame();
        return true;
    }
    return false;
}

bool basicAnimationUnsquish(Animator& animator, ecs::Entity entity) {
    Transform2D& trans = entity.get<Transform2D>();

    const f32 unsquishStep = System::dt();
    trans.scale = {math::approach(trans.scale.x, 1.0, unsquishStep), math::approach(trans.scale.y, 1.0, unsquishStep)};
    return basicAnimation(animator, entity);
}

Animator::Animator(AnimInfo animInfo, bool isLooping_) : isLooping(isLooping_) {
    loadAnimations(*this, animInfo);
}

Animator::Animator(AnimInfo animInfo, AnimBrain brain_, bool isLooping_) : brain(brain_), isLooping(isLooping_) {
    loadAnimations(*this, animInfo);
}

Frame Animator::getFrame() const {
    return animations[curAnimIx].getFrame(curFrameIx);
}

Animation& Animator::getAnimation() {
    return animations[curAnimIx];
}

bool Animator::setAnimation(s32 id) {
    if (getAnimation().id == id) {
        return false;
    }

    for (size_t i = 0; i < animations.size(); i++) {
        if (animations[i].id == id) {
            curAnimIx = i;
            resetAnimation();
            return true;
        }
    }
    print("failed to set animation ID:", id);
    return false;  // return error?
}

void Animator::nextFrame() {
    if (curFrameIx + 1 == getAnimation().getFrameCount() && !isLooping) {
        return;
    }
    curFrameIx = (curFrameIx + 1) % getAnimation().getFrameCount();
    curFrameDuration = 0.0;
}

void Animator::resetAnimation() {
    curAnimDuration = 0;
    curFrameIx = 0;
    curFrameDuration = System::rng.uniform() * 0.5;
}

void Animator::setLooping(bool loop) {
    isLooping = loop;
}

Animation::Animation() : frames({}), id(-1) {};

Animation::Animation(s32 id_, std::vector<Frame> frames_, f32 secondsPerFrame_) : frames(frames_), id(id_), secondsPerFrame(secondsPerFrame_) {}

Frame Animation::getFrame(s32 ix) const {
    return frames[ix];
}

s32 Animation::getFrameCount() const {
    return frames.size();
}

}  // namespace whal
