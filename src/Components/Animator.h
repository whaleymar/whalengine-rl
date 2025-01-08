#pragma once

#include <vector>

#include "Map/ComponentFactory.h"
#include "Util/Types.h"

#include "Gfx/Frame.h"

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

// each element contains:
// 1. the animation's base name (e.g. "player-idle")
// 2. the animation ID
// 3. the number of frames in the animation
// 4. the number of seconds per frame
using AnimInfo = std::vector<std::tuple<const char*, s32, s32, f32>>;
struct Animator : ISerialize<Animator, ComponentFactory> {
    Animator() = default;
    Animator(AnimInfo animInfo, bool isLooping_ = true);
    Animator(const Animation& animation, bool isLooping_ = true);  // convenience constructor for animators with one animation
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

    static void loadImpl(ecs::Entity entity, const LoadContext& ctx);
};

// an animation is a sequence of same-sized frames
// TODO enable variable frametimes
// TODO replace ID with name
struct Animation {
    Animation();
    Animation(s32 id, std::vector<Frame> frames, f32 secondsPerFrame = 0.25);
    Animation(const char* basename, s32 id, s32 frameCount, f32 secondsPerFrame);

    Frame getFrame(s32 ix) const;
    s32 getFrameCount() const;

    std::vector<Frame> frames;
    s32 id;
    f32 secondsPerFrame = 0.25;
};

}  // namespace whal
