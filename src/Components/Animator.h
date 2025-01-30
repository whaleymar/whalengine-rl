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
    static Animator fromAnimation(const Animation& animation, AnimBrain brain = &basicAnimation, bool isLooping = true);
    static Animator fromAnimation(const char* factoryName, AnimBrain brain = &basicAnimation, bool isLooping = true);
    static Animator from(const AnimInfo& animInfo, AnimBrain brain, bool isLooping = true);

    std::vector<Animation> animations;
    AnimBrain brain = &basicAnimation;
    s32 curAnimIx = 0;
    f32 curAnimDuration = 0.0;
    s32 curFrameIx = 0;
    f32 _curFrameDuration = 0.0;
    bool isLooping = true;

    Frame getFrame() const;
    Animation& getAnimation();
    s32 getFrameIx() const;
    f32 getFrameTimeElapsed() const;
    f32 getFrameDuration() const;
    bool isFrameDone() const;

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

    std::vector<FrameExt> frames;
    std::string name;

    s32 getFrameCount() const;
};

}  // namespace whal
