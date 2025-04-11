#pragma once

#include "Gfx/Shader.h"

namespace whal {

class Posterize : public IShaderProcess {
    SINGLETON(Posterize)
public:
    void process(rl::RenderTexture source, rl::RenderTexture dest) override;

#ifndef NDEBUG
    void drawEditor() override;
#endif
};

}  // namespace whal
