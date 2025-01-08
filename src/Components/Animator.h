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
// 1. the animation's name as registered in the AnimationFactory (typically the basename of the file, like "sprite/player.aseprite" -> "player").
// 2. the animation's simple name, which is used to play it with Animator::play. Pass an empty string to use the animation's full name.
using AnimInfo = std::vector<std::tuple<const char*, const char*>>;
struct Animator : ISerialize<Animator, ComponentFactory> {
    Animator() = default;
    Animator(const Animation& animation, bool isLooping_ = true);  // convenience constructor for animators with one animation
    Animator(const AnimInfo& animInfo, AnimBrain brain_, bool isLooping_ = true);

    std::vector<Animation> animations;
    AnimBrain brain = &basicAnimation;
    s32 curAnimIx = 0;
    f32 curAnimDuration = 0.0;
    bool isLooping = true;

    Frame getFrame() const;
    Animation& getAnimation();
    s32 getFrameIx() const;
    f32 getFrameTimeElapsed() const;

    bool play(const std::string& name);
    bool isPlaying(const std::string& name) const;
    bool isPlaying(const std::initializer_list<std::string>& names) const;
    void nextFrame();
    void resetAnimation();
    void setLooping(bool loop);

    static void loadImpl(ecs::Entity entity, const LoadContext& ctx);
};

// an animation is a sequence of same-sized frames
struct Animation {
    struct FrameExt {
        Frame frame;
        f32 duration;
    };

    Animation();
    Animation(const char* basename, std::vector<FrameExt> frames);
    Animation(const char* basename, s32 frameCount, f32 secondsPerFrame);

    Frame getFrame() const;
    f32 getFrameDuration() const;
    s32 getFrameCount() const;
    bool isFrameDone() const;

    void nextFrame(bool isLooping);
    void reset();

    std::vector<FrameExt> frames;
    std::string name;
    s32 curFrameIx = 0;
    f32 curFrameDuration = 0.0;
};

}  // namespace whal
