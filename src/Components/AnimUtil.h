#pragma once

#include <vector>

#include "Util/Types.h"

namespace whal {

struct Animator;

// basename, ID, nFrames, secondsPerFrame
using AnimInfo = std::vector<std::tuple<const char*, s32, s32, f32>>;

void loadAnimations(Animator& animator, const AnimInfo& animInfo);

}  // namespace whal
