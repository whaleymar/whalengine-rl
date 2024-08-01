#pragma once

#include "Util/Types.h"

typedef struct Shader Shader;
class Game;

namespace whal {

enum class Shaders : s16 {
    Default = 0,
    PointLight,
    BoxLight,
    Radiance,
    Silhouette,
    Quantize,
    Outline,
    Bloom,
    Glow,
    ToneMap,
    Glitch,
    ShadowLight,
    Blur,
    _Count_DO_NOT_USE_ME
};

class ScopedShader {
public:
    ScopedShader(Shader shader, bool isActivated = false);
    ~ScopedShader();
};

class ShaderManager {
    friend Game;

public:
    static ShaderManager& instance() {
        static ShaderManager instance_;
        return instance_;
    }

    static Shader get(Shaders shaderEnum);
    static void activate(Shaders shaderEnum);
    static ScopedShader activateScoped(Shaders shaderEnum);

private:
    void loadShaders();
    void unloadAll();

    Shader _get(Shaders shaderEnum) const;
    void setIsUsed(s32 index);
    bool getIsUsed(s32 index) const;

    u32 mUsageMask = 0;
};

}  // namespace whal
