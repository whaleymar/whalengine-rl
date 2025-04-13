#include "Animator.h"

#include <cassert>
#include <cstring>

#include "Components/Draw.h"
#include "Components/Transform.h"
#include "ECS.h"
#include "Map/AnimationFactory.h"
#include "Sys/System.h"
#include "Sys/Time.h"
#include "Util/Print.h"

namespace whal {

bool basicAnimation(Animator& animator, ecs::Entity entity) {
    animator._curFrameDuration += Time.dt();
    animator.curAnimDuration += Time.dt();
    if (animator.isFrameDone()) {
        animator.nextFrame();
        return true;
    }
    return false;
}

bool basicAnimationUnsquish(Animator& animator, ecs::Entity entity) {
    Transform& trans = entity.get<Transform>();

    const f32 unsquishStep = Time.dt();
    trans.setScale({math::approach(trans.scale.x, math::sign(trans.scale.x), unsquishStep),
                    math::approach(trans.scale.y, math::sign(trans.scale.y), unsquishStep)},
                   entity);
    return basicAnimation(animator, entity);
}

Animator Animator::fromAnimation(const Animation& animation, AnimBrain brain, bool isLooping) {
    Animator animator = Animator{
        .animations = {animation},
        .brain = brain,
        .isLooping = isLooping,
    };
    animator.resetAnimation();
    return animator;
}

Animator Animator::fromAnimation(const char* factoryName, AnimBrain brain, bool isLooping) {
    return Animator::fromAnimation(AnimationFactory::get(factoryName), brain, isLooping);
}

Animator Animator::from(const AnimInfo& animInfo, AnimBrain brain, bool isLooping) {
    Animator animator = Animator{
        .brain = brain,
        .isLooping = isLooping,
    };
    for (const auto& [animBaseName, animName] : animInfo) {
        Animation anim = AnimationFactory::get(animBaseName);
        if (strlen(animName)) {
            anim.name = animName;
        }
        animator.animations.push_back(anim);
    }
    animator.resetAnimation();
    return animator;
}

Frame Animator::getFrame() const {
    return animations[curAnimIx].frames[curFrameIx].frame;
}

Animation& Animator::getAnimation() {
    return animations[curAnimIx];
}

s32 Animator::getFrameIx() const {
    return curFrameIx;
}

f32 Animator::getFrameTimeElapsed() const {
    return _curFrameDuration;
}

f32 Animator::getFrameDuration() const {
    return animations[curAnimIx].frames[curFrameIx].duration;
}

bool Animator::isFrameDone() const {
    return _curFrameDuration >= animations[curAnimIx].frames[curFrameIx].duration;
}

bool Animator::isAnimationJustFinished() const {
    return _isAnimationFinishedThisFrame;
}

f32 Animator::getAnimationTimeRemaining() const {
    f32 timeLeftCeiling = animations[curAnimIx].getRemaining(curFrameIx);
    return timeLeftCeiling - (_curFrameDuration < 0.0f ? 0.0f : _curFrameDuration);
}

bool Animator::play(const std::string& name) {
    if (getAnimation().name == name) {
        return false;
    }

    _isAnimationFinishedThisFrame = false;
    _isLoopingAnimationDone = false;
    for (size_t i = 0; i < animations.size(); i++) {
        if (animations[i].name == name) {
            curAnimIx = i;
            resetAnimation();
            return true;
        }
    }
    print("No animation found with name", name);
    return false;  // return error?
}

bool Animator::isPlaying(const std::string& name) const {
    return animations[curAnimIx].name == name;
}

bool Animator::isPlaying(const std::initializer_list<std::string>& names) const {
    for (const std::string& name : names) {
        if (name == animations[curAnimIx].name) {
            return true;
        }
    }
    return false;
}

void Animator::nextFrame() {
    _isAnimationFinishedThisFrame = false;
    const s32 frameCount = getAnimation().getFrameCount();
    if (curFrameIx + 1 == frameCount && !isLooping) {
        // animation is done
        if (!_isLoopingAnimationDone) {
            _isAnimationFinishedThisFrame = true;
        }
        _isLoopingAnimationDone = true;
        return;
    }
    curFrameIx = (curFrameIx + 1) % frameCount;
    _curFrameDuration = 0.0;
    _isAnimationFinishedThisFrame = curFrameIx == 0;
}

void Animator::resetAnimation() {
    _isAnimationFinishedThisFrame = false;
    _isLoopingAnimationDone = false;
    curAnimDuration = 0;
    curFrameIx = 0;
    _curFrameDuration = Rng.uniform() * 0.5;
}

void Animator::setLooping(bool loop) {
    isLooping = loop;
}

s32 Animation::getFrameCount() const {
    return frames.size();
}

void Animation::setDuration(f32 totalDuration) {
    const f32 currentDuration = getDuration();
    const f32 mult = totalDuration / currentDuration;
    for (FrameExt& frame : frames) {
        frame.duration *= mult;
    }
}

f32 Animation::getDuration() const {
    f32 currentDuration = 0.0f;
    for (const FrameExt& frame : frames) {
        currentDuration += frame.duration;
    }
    return currentDuration;
}

f32 Animation::getRemaining(s32 ix) const {
    f32 currentDuration = 0.0f;
    for (u64 i = ix; i < frames.size(); i++) {
        currentDuration += frames[i].duration;
    }
    return currentDuration;
}

}  // namespace whal
