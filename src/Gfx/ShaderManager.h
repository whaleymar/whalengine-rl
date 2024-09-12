#pragma once

#include "Util/Types.h"

typedef struct Shader Shader;

namespace whal {

enum class Shaders : s16 {
    Default = 0,
    PointLight,
    BoxLight,
    Radiance,
    Silhouette,
    Quantize,
    // ToneMap,
    Glitch,
    ShadowLight,
    Blur,
    PostProcess,
    EffectsMask,
    _Count_DO_NOT_USE_ME
};

class ScopedShader {
public:
    ScopedShader(Shader shader, bool isActivated = false);
    ~ScopedShader();
};

class ShaderManager {
public:
    static ShaderManager& instance() {
        static ShaderManager instance_;
        return instance_;
    }

    static Shader get(Shaders shaderEnum);
    static void activate(Shaders shaderEnum);
    static ScopedShader activateScoped(Shaders shaderEnum);

    void loadShaders();
    void unloadAll();
    void reloadShaders();

private:
    Shader _get(Shaders shaderEnum) const;
    void setIsUsed(s32 index);
    bool getIsUsed(s32 index) const;

    u32 mUsageMask = 0;
};

}  // namespace whal
