#include "Animator.h"

#include <cassert>
#include <cstring>
#include "Map/TiledParse.h"
#include "whalECS/src/ECS.h"

#include "Components/Draw.h"

#include "Gfx/Texture.h"

#include "Map/AnimationFactory.h"

#include "Sys/System.h"

#include "Util/Print.h"

namespace whal {

static void loadAnimations(Animator& animator, const AnimInfo& animInfo) {
    for (auto [animBaseName, id, count, secsPerFrame] : animInfo) {
        animator.animations.push_back(Animation(animBaseName, id, count, secsPerFrame));
    }
    animator.resetAnimation();
}

bool basicAnimation(Animator& animator, ecs::Entity entity) {
    const Animation& anim = animator.getAnimation();

    // play animations at normal speed if paused? Seems cute
    animator.curFrameDuration += Time.dt();
    animator.curAnimDuration += Time.dt();
    if (animator.curFrameDuration >= anim.secondsPerFrame) {
        animator.nextFrame();
        return true;
    }
    return false;
}

bool basicAnimationUnsquish(Animator& animator, ecs::Entity entity) {
    Transform& trans = entity.get<Transform>();

    const f32 unsquishStep = Time.dt();
    trans.scale = {math::approach(trans.scale.x, 1.0, unsquishStep), math::approach(trans.scale.y, 1.0, unsquishStep)};
    return basicAnimation(animator, entity);
}

Animator::Animator(AnimInfo animInfo, bool isLooping_) : isLooping(isLooping_) {
    loadAnimations(*this, animInfo);
}

Animator::Animator(const Animation& animation, bool isLooping_) : animations({animation}), isLooping(isLooping_) {
    resetAnimation();
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
    curFrameDuration = Rng.uniform() * 0.5;
}

void Animator::setLooping(bool loop) {
    isLooping = loop;
}

Animation::Animation() : frames({}), id(-1) {};

Animation::Animation(s32 id_, std::vector<Frame> frames_, f32 secondsPerFrame_) : frames(frames_), id(id_), secondsPerFrame(secondsPerFrame_) {}

Animation::Animation(const char* basename, s32 id_, s32 frameCount, f32 secondsPerFrame_) : id(id_), secondsPerFrame(secondsPerFrame_) {
    const auto& spriteTexture = TextureManager::getAtlas(TEXNAME_SPRITE);
    for (s32 i = 0; i < frameCount; i++) {
        auto animName = whal_format("{}{}", basename, i + 1);

        auto frame = spriteTexture.getFrame(animName.c_str());
#ifndef NDEBUG
        if (!frame) {
            print("Failed to load animation frame:", animName);
            assert(false);
        }
#endif
        frames.push_back(*frame);
    }
}

Frame Animation::getFrame(s32 ix) const {
    return frames[ix];
}

s32 Animation::getFrameCount() const {
    return frames.size();
}

void Animator::loadImpl(ecs::Entity entity, const LoadContext& ctx) {
    Sprite sprite = entity.has<Sprite>() ? entity.get<Sprite>() : Sprite{};
    std::string animatorName = readString(*ctx.values, "Animator");
    Animator animator = Animator(AnimationFactory::get(animatorName.c_str()));
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

}  // namespace whal
