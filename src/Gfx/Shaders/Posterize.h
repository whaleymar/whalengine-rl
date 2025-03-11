#pragma once

#include "Gfx/Shader.h"

namespace whal {

class Posterize : public IShaderProcess {
public:
    Posterize();
    void process(rl::RenderTexture source, rl::RenderTexture dest) override;

#ifndef NDEBUG
    void drawDebug() override;
#endif
};

}  // namespace whal
