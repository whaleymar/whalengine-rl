#pragma once

#include "Gfx/Shader.h"
namespace whal {

class Tonemap : public IShaderProcess {
public:
    Tonemap();
    void process(rl::RenderTexture source, rl::RenderTexture dest) override;
};

}  // namespace whal
