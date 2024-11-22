#pragma once

#include "Gfx/BaseShader.h"
namespace whal {

class Tonemap : public IShader {
public:
    Tonemap();
    void process(RenderTexture source, RenderTexture dest) override;

private:
    BaseShader mToneMap;
};

}  // namespace whal
