#pragma once

#include "Gfx/BaseShader.h"
namespace whal {

class Bloom : public BaseShader {
public:
    Bloom();
    void process(RenderTexture source, RenderTexture dest) override;

    f32 threshold = 1.5;
};

}  // namespace whal
