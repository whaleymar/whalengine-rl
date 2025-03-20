#pragma once

#include <string>

namespace whal {

struct Animation;

class AnimationFactory {
public:
    static void add(const std::string& name, const Animation& animation);
    static const Animation& get(const char* name);
};

}  // namespace whal
