#pragma once

#include <string>
#include <unordered_map>
#include "Components/Animator.h"

namespace whal {

class AnimationFactory {
public:
    static AnimationFactory& instance() {
        static AnimationFactory instance_;
        return instance_;
    }

    static void add(const char* name, Animator animator);
    static Animator get(const char* name);

private:
    std::unordered_map<std::string, Animator> mTable;
};

}  // namespace whal
