#pragma once

namespace whal {

struct Animation;

class AnimationFactory {
public:
    static void add(const char* name, const Animation& animation);
    static const Animation& get(const char* name);
};

}  // namespace whal
