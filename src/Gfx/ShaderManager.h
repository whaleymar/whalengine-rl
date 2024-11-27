#pragma once

#include "Util/Types.h"

namespace rl {
typedef struct Shader Shader;
}

namespace whal {

enum class Shaders : s16 {
    Default = 0,
    PointLight,
    BoxLight,
    // Quantize,
    ToneMap,
    ShadowLight,
    Blur,
    BlurLowRes,
    LightPassThru,
    Pixelate,
    Test,
    _Count_DO_NOT_USE_ME
};

class ScopedShader {
public:
    ScopedShader(rl::Shader shader, bool isActivated = false);
    ~ScopedShader();
};

class ShaderManager {
public:
    static ShaderManager& instance() {
        static ShaderManager instance_;
        return instance_;
    }

    static rl::Shader get(Shaders shaderEnum);
    static void activate(Shaders shaderEnum);
    static ScopedShader activateScoped(Shaders shaderEnum);

    void loadShaders();
    void unloadAll();
    void reloadShaders();

private:
    rl::Shader _get(Shaders shaderEnum) const;
    void setIsUsed(s32 index);
    bool getIsUsed(s32 index) const;

    u32 mUsageMask = 0;
};

}  // namespace whal
