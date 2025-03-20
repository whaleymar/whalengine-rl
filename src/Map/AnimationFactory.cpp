#include "AnimationFactory.h"

#include <string>
#include <unordered_map>
#include "Components/Animator.h"

namespace whal {

std::unordered_map<std::string, Animation> S_ANIMATION_TABLE;

void AnimationFactory::add(const std::string& name, const Animation& animation) {
    S_ANIMATION_TABLE.insert({name, animation});
}

const Animation& AnimationFactory::get(const char* name) {
    assert(S_ANIMATION_TABLE.contains(name));
    return S_ANIMATION_TABLE[name];
}

}  // namespace whal
