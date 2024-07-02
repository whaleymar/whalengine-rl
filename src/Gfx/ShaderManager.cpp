#include "ShaderManager.h"

#include <array>
#include <cassert>
#include <raylib.h>

namespace whal {

static std::array<Shader, static_cast<s32>(Shaders::_Count_DO_NOT_USE_ME)> S_SHADERS;

ShaderManager::ShaderManager() {
    struct ShaderInfo {
        Shaders shaderEnum;
        const char* vertexPath;
        const char* fragPath;
    };

    static const ShaderInfo shaderInfo[] = {
        {Shaders::Default, 0, 0},
        {Shaders::PointLight, 0, "src/Shader/pointlight.glsl"},
        {Shaders::Radiance, 0, "src/Shader/radiancelight.glsl"},
        {Shaders::Silhouette, 0, "src/Shader/silhouette.glsl"},
        {Shaders::Quantize, 0, "src/Shader/quantize.glsl"},
        {Shaders::Outline, 0, "src/Shader/outline.glsl"},
    };

    constexpr s32 len = sizeof(shaderInfo) / sizeof(ShaderInfo);

    for (size_t i = 0; i < len; i++) {
        // dynamic allocation
        Shader shader = LoadShader(shaderInfo[i].vertexPath, shaderInfo[i].fragPath);
        s32 ix = static_cast<s32>(shaderInfo[i].shaderEnum);
        S_SHADERS[ix] = shader;
        setIsUsed(ix);
    }
}

ShaderManager::~ShaderManager() {
    // idk why this is segfaulting. this runs on program close so maybe the memory is free'd somehow during shutdown
    // const s32 maxShaderCount = static_cast<s32>(Shaders::_Count_DO_NOT_USE_ME);
    // for (size_t i = 0; i < maxShaderCount; i++) {
    //     if (getIsUsed(i)) {
    //         UnloadShader(S_SHADERS[i]);
    //     }
    // }
}

Shader ShaderManager::get(Shaders shaderEnum) {
    return instance()._get(shaderEnum);
}

Shader ShaderManager::_get(Shaders shaderEnum) const {
    s32 ix = static_cast<s32>(shaderEnum);
    assert(getIsUsed(ix) && "Shader not registered for passed enum");
    return S_SHADERS[ix];
}

void ShaderManager::setIsUsed(s32 index) {
    mUsageMask |= (1 << index);
}

bool ShaderManager::getIsUsed(s32 index) const {
    return (mUsageMask & (1 << index)) > 0;
}

}  // namespace whal
