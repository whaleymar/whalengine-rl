#pragma once

#include "Gfx/Shader.h"
#include "Util/Singleton.h"

namespace whal {

class LightDenoise : public IShaderProcess {
    SINGLETON(LightDenoise)
public:
    void process(rl::RenderTexture source, rl::RenderTexture dest) override;

    // doesn't work as uniform, shader needs it to be const
    // s32 kernelSize = 3;  // MINIMUM 3, SHOULD BE ODD NUMBER
};

}  // namespace whal
