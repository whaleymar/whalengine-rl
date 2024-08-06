#include "ShaderManager.h"

#include <array>
#include <cassert>
#include <raylib.h>
#include "Settings.h"
#include "Sys/System.h"

namespace whal {

struct Uniforms {
    enum flags : u32 {
        None = 0,
        Time = 1,
        Resolution = 1 << 1,
        Palette = 1 << 2,
    };

    bool isSet(flags flag) { return (uniformFlags & flag) > 0; }

    u32 uniformFlags = None;
    int iTime;
    int iResolution;
    int iPalette;
};

static std::array<Shader, static_cast<s32>(Shaders::_Count_DO_NOT_USE_ME)> S_SHADERS;
static std::array<Uniforms, static_cast<s32>(Shaders::_Count_DO_NOT_USE_ME)> S_UNIFORMS;

static void ActivateShader(Shaders shaderEnum) {
    auto shader = ShaderManager::get(shaderEnum);
    s32 ix = static_cast<s32>(shaderEnum);
    Uniforms uniforms = S_UNIFORMS[ix];

    BeginShaderMode(shader);

    if (uniforms.isSet(Uniforms::Time)) {
        const f32 iTime = System::time.getElapsed();
        SetShaderValue(shader, uniforms.iTime, &iTime, SHADER_UNIFORM_FLOAT);
    }

    if (uniforms.isSet(Uniforms::Resolution)) {
        const f32 iResolution[2] = {FWINDOW_WIDTH_PIXELS, FWINDOW_HEIGHT_PIXELS};
        SetShaderValue(shader, uniforms.iResolution, &iResolution, SHADER_UNIFORM_VEC2);
    }

    // not working
    // if (uniforms.isSet(Uniforms::Palette)) {
    //     SetShaderValueTexture(shader, uniforms.iPalette, TextureManager::instance().getTexture(TEXNAME_PALETTE));
    // }
}

ScopedShader::ScopedShader(Shader shader, bool isActivated) {
    if (!isActivated) {
        BeginShaderMode(shader);
    }
}

ScopedShader::~ScopedShader() {
    EndShaderMode();
}

void ShaderManager::loadShaders() {
    struct ShaderInfo {
        Shaders shaderEnum;
        const char* vertexPath;
        const char* fragPath;
        u32 uniformFlags = Uniforms::None;
    };

    static const ShaderInfo shaderInfo[] = {
        {Shaders::Default, 0, 0},
        {Shaders::PointLight, 0, "src/Shader/pointlight.glsl"},
        {Shaders::BoxLight, 0, "src/Shader/aabblight.glsl"},
        {Shaders::Radiance, 0, "src/Shader/radiancelight.glsl"},
        {Shaders::Silhouette, 0, "src/Shader/silhouette.glsl"},
        {Shaders::Quantize, 0, "src/Shader/quantize.glsl", Uniforms::Palette},
        {Shaders::Outline, 0, "src/Shader/outline.glsl"},
        {Shaders::Bloom, 0, "src/Shader/bloom.glsl", Uniforms::Resolution},
        {Shaders::Glow, 0, "src/Shader/glow.glsl"},
        // {Shaders::ToneMap, 0, "src/Shader/toneMapping.glsl"},
        {Shaders::Glitch, 0, "src/Shader/glitch-ppEffect.glsl", Uniforms::Time | Uniforms::Resolution},
        {Shaders::ShadowLight, 0, "src/Shader/shadowLight.glsl", Uniforms::Time | Uniforms::Resolution},
        {Shaders::Blur, 0, "src/Shader/blur.glsl", Uniforms::Resolution},
    };

    constexpr s32 len = sizeof(shaderInfo) / sizeof(ShaderInfo);

    for (size_t i = 0; i < len; i++) {
        // dynamic allocation
        // TODO STUBBED
        // Shader shader = LoadShader(shaderInfo[i].vertexPath, shaderInfo[i].fragPath);
        Shader shader = LoadShader(0, 0);
        s32 ix = static_cast<s32>(shaderInfo[i].shaderEnum);
        S_SHADERS[ix] = shader;
        setIsUsed(ix);

        S_UNIFORMS[ix].uniformFlags = shaderInfo[i].uniformFlags;

        if (S_UNIFORMS[ix].isSet(Uniforms::Time)) {
            S_UNIFORMS[ix].iTime = GetShaderLocation(shader, "iTime");
        }
        if (S_UNIFORMS[ix].isSet(Uniforms::Resolution)) {
            S_UNIFORMS[ix].iResolution = GetShaderLocation(shader, "iResolution");
        }
        // if (S_UNIFORMS[ix].isSet(Uniforms::Palette)) {
        //     S_UNIFORMS[ix].iPalette = GetShaderLocation(shader, "iPalette");
        // }
    }
}

void ShaderManager::unloadAll() {
    // idk why this is segfaulting. this runs on program close so maybe the memory is free'd somehow during shutdown
    const s32 maxShaderCount = static_cast<s32>(Shaders::_Count_DO_NOT_USE_ME);
    for (size_t i = 0; i < maxShaderCount; i++) {
        if (getIsUsed(i)) {
            UnloadShader(S_SHADERS[i]);
        }
    }
}

Shader ShaderManager::get(Shaders shaderEnum) {
    return instance()._get(shaderEnum);
}

void ShaderManager::activate(Shaders shaderEnum) {
    ActivateShader(shaderEnum);
}

ScopedShader ShaderManager::activateScoped(Shaders shaderEnum) {
    ActivateShader(shaderEnum);
    return ScopedShader(instance()._get(shaderEnum), true);
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
