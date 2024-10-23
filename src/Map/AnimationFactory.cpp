#include "AnimationFactory.h"

namespace whal {

void AnimationFactory::add(const char* name, Animator animator) {
    instance().mTable.insert({std::string(name), animator});
}

Animator AnimationFactory::get(const char* name) {
    assert(instance().mTable.contains(name));
    return instance().mTable[name];
}

}  // namespace whal
