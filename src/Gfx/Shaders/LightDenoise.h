#pragma once

#include "Gfx/Shader.h"
namespace whal {

class LightDenoise : public IShaderProcess {
public:
    LightDenoise();
    void process(rl::RenderTexture source, rl::RenderTexture dest) override;

    // doesn't work as uniform, shader needs it to be const
    // s32 kernelSize = 3;  // MINIMUM 3, SHOULD BE ODD NUMBER

private:
    Shader mDenoise;
};

}  // namespace whal
