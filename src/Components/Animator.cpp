#include "Animator.h"

#include <cassert>
#include <cstring>

#include "ECS.h"

#include "Components/Draw.h"

#include "Map/AnimationFactory.h"
#include "Map/TiledParse.h"

#include "Sys/System.h"

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

bool Animator::play(const std::string& name) {
    if (getAnimation().name == name) {
        return false;
    }

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
    const s32 frameCount = getAnimation().getFrameCount();
    if (curFrameIx + 1 == frameCount && !isLooping) {
        // animation is done
        return;
    }
    curFrameIx = (curFrameIx + 1) % frameCount;
    _curFrameDuration = 0.0;
}

void Animator::resetAnimation() {
    curAnimDuration = 0;
    curFrameIx = 0;
    _curFrameDuration = Rng.uniform() * 0.5;
}

void Animator::setLooping(bool loop) {
    isLooping = loop;
}

void Animator::loadImpl(ecs::Entity entity, const LoadContext& ctx) {
    Sprite sprite = entity.has<Sprite>() ? entity.get<Sprite>() : Sprite{};
    std::string animatorName = readString(*ctx.values, "Animator");
    Animator animator = Animator::fromAnimation(AnimationFactory::get(animatorName.c_str()));
    entity.add(animator);
    sprite.setFrame(animator.getFrame());

    s32 rotation;
    if (tryRead(*ctx.values, "rotationDegrees", &rotation)) {
        entity.get<Transform>().rotation = rotation;
    }

    tryRead(*ctx.values, "Color", &sprite.color);

    f32 brightness;
    if (tryRead(*ctx.values, "Brightness", &brightness)) {
        sprite.color.scale(brightness);
    }
    entity.add(sprite);
}

s32 Animation::getFrameCount() const {
    return frames.size();
}

}  // namespace whal
