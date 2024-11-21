#pragma once

#include "Gfx/BaseShader.h"
namespace whal {

class LightDenoise : public BaseShader {
public:
    LightDenoise();
    void process(RenderTexture source, RenderTexture dest) override;

    // doesn't work as uniform, shader needs it to be const
    // s32 kernelSize = 3;  // MINIMUM 3, SHOULD BE ODD NUMBER
};

}  // namespace whal
