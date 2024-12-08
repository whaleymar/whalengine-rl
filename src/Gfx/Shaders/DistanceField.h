#pragma once

#include "Gfx/Shader.h"
namespace whal {

class DistanceField : public IShaderProcess {
public:
    DistanceField();
    void process(rl::RenderTexture source, rl::RenderTexture dest) override;

private:
    Shader mUvMask;
    Shader mJumpFlood;
    Shader mDistanceField;
};

}  // namespace whal
