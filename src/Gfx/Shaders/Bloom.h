#pragma once

#include "Gfx/BaseShader.h"
namespace whal {

class Bloom : public IShader {
public:
    Bloom();
    void process(RenderTexture source, RenderTexture dest) override;

    f32 threshold = 1.5;  // [Range(0.0f, 10.0f)]

    // When softThreshold is 0, the shader has a "hard knee" & there is no gradient
    // between bloomed and non-bloomed areas. At 1, brightness values all the way from 0 are smoothly bloomed.
    f32 softThreshold = 0.5;  // [Range(0.0f, 1.0f)]

    f32 intensity = 1.0;  // [Range(0.0f, 10.0f)]

private:
    BaseShader mThresh;
    BaseShader mBlur;
};

}  // namespace whal
