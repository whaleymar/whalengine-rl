#pragma once

#include <vector>

namespace whal {

class IShaderProcess;

// TODO i want a "pre-lighting" shader list
struct Camera {
    std::vector<IShaderProcess*> postEffects;
    // std::vector<IShaderProcess*> lightEffects;
};

}  // namespace whal
