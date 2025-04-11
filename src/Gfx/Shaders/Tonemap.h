#pragma once

#include "Gfx/Shader.h"
#include "Util/Singleton.h"

namespace whal {

class Tonemap : public IShaderProcess {
    SINGLETON_CUSTOM(Tonemap)
public:
    void process(rl::RenderTexture source, rl::RenderTexture dest) override;
};

}  // namespace whal
