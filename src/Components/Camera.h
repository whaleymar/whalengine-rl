#pragma once

#include <vector>

namespace whal {

class IShaderProcess;

struct Camera {
    std::vector<IShaderProcess*> beforeLighting;
    std::vector<IShaderProcess*> postEffects;
    IShaderProcess* lightingUpscaler = nullptr;  // shader used when scaling lights from game to render resolution
    // std::vector<IShaderProcess*> lightEffects; // would probably want to split into pre/post upscale stages
};

}  // namespace whal
