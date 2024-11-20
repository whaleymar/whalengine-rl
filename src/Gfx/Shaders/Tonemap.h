#pragma once

#include "Gfx/BaseShader.h"
namespace whal {

class Tonemap : public BaseShader {
public:
    Tonemap();
    void process(RenderTexture source, RenderTexture dest) override;
};

}  // namespace whal
