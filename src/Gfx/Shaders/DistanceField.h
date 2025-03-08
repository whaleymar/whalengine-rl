#pragma once

#include "Gfx/Shader.h"
namespace whal {

class DistanceField : public IShaderProcess {
public:
    DistanceField();
    void process(rl::RenderTexture source, rl::RenderTexture dest) override;
#ifndef NDEBUG
    void drawDebug() override;
#endif
};

}  // namespace whal
