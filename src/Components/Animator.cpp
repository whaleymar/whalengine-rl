#include "Animator.h"

#include <cassert>
#include <cstring>

#include "ECS.h"

#include "Components/Draw.h"

#include "Gfx/Texture.h"

#include "Map/AnimationFactory.h"
#include "Map/TiledParse.h"

#include "Sys/System.h"

#include "Util/Print.h"

namespace whal {

static void loadAnimations(Animator& animator, const AnimInfo& animInfo) {
    for (auto [animBaseName, animName] : animInfo) {
        Animation anim = AnimationFactory::get(animBaseName);
        anim.name = animName;
        animator.animations.push_back(anim);
    }
    animator.resetAnimation();
}

bool basicAnimation(Animator& animator, ecs::Entity entity) {
    Animation& anim = animator.getAnimation();

    anim.curFrameDuration += Time.dt();
    animator.curAnimDuration += Time.dt();
    if (anim.curFrameDuration >= anim.getFrameDuration()) {
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

Animator::Animator(const Animation& animation, bool isLooping_) : animations({animation}), isLooping(isLooping_) {
    resetAnimation();
}

Animator::Animator(const AnimInfo& animInfo, AnimBrain brain_, bool isLooping_) : brain(brain_), isLooping(isLooping_) {
    loadAnimations(*this, animInfo);
}

Frame Animator::getFrame() const {
    return animations[curAnimIx].getFrame();
}

Animation& Animator::getAnimation() {
    return animations[curAnimIx];
}

s32 Animator::getFrameIx() const {
    return animations[curAnimIx].curFrameIx;
}

f32 Animator::getFrameTimeElapsed() const {
    return animations[curAnimIx].curFrameDuration;
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
    getAnimation().nextFrame(isLooping);
}

void Animator::resetAnimation() {
    curAnimDuration = 0;
    getAnimation().reset();
}

void Animator::setLooping(bool loop) {
    isLooping = loop;
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

Animation::Animation() : frames({}) {};

Animation::Animation(const char* basename, std::vector<FrameExt> frames_) : frames(frames_), name(basename) {}

Animation::Animation(const char* basename, s32 frameCount, f32 secondsPerFrame_) : name(basename) {
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
        frames.push_back(FrameExt{
            .frame = *frame,
            .duration = secondsPerFrame_,
        });
    }
}

Frame Animation::getFrame() const {
    return frames[curFrameIx].frame;
}

f32 Animation::getFrameDuration() const {
    return frames[curFrameIx].duration;
}

s32 Animation::getFrameCount() const {
    return frames.size();
}

void Animation::nextFrame(bool isLooping) {
    if (curFrameIx + 1 == getFrameCount() && !isLooping) {
        // animation is done
        return;
    }
    curFrameIx = (curFrameIx + 1) % getFrameCount();
    curFrameDuration = 0.0;
}

void Animation::reset() {
    curFrameIx = 0;
    curFrameDuration = Rng.uniform() * 0.5;
}

}  // namespace whal
