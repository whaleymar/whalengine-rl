#pragma once

#include "Gfx/Shader.h"
#include "Util/Singleton.h"

namespace whal {

class DistanceField : public IShaderProcess {
    SINGLETON_CUSTOM(DistanceField)
public:
    void process(rl::RenderTexture source, rl::RenderTexture dest) override;
#ifndef NDEBUG
    void drawEditor() override;
#endif
};

}  // namespace whal
