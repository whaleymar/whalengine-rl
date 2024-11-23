#pragma once

#include "Gfx/BaseShader.h"
namespace whal {

class Tonemap : public IShader {
public:
    Tonemap();
    void process(rl::RenderTexture source, rl::RenderTexture dest) override;

private:
    BaseShader mToneMap;
};

}  // namespace whal
