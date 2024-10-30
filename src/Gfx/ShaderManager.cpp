#include "ShaderManager.h"

#include <array>
#include <cassert>
#include <cstring>
#include <raylib.h>
#include "Settings.h"
#include "Sys/System.h"

#ifdef __EMSCRIPTEN__
#include "Util/Print.h"
#endif

namespace whal {

struct Uniforms {
    enum flags : u32 {
        None = 0,
        Time = 1,
        Resolution = 1 << 1,
        VirtualResolution = 1 << 2,  // mutually exclusive w/ Resolution
        Palette = 1 << 3,
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

    if (uniforms.isSet(Uniforms::VirtualResolution)) {
        const f32 iResolution[2] = {FWINDOW_WIDTH_GAME, FWINDOW_HEIGHT_GAME};
        SetShaderValue(shader, uniforms.iResolution, &iResolution, SHADER_UNIFORM_VEC2);
    } else if (uniforms.isSet(Uniforms::Resolution)) {
        const f32 iResolution[2] = {FWINDOW_WIDTH_RENDER, FWINDOW_HEIGHT_RENDER};
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
        {Shaders::BoxLight, 0, "src/Shader/aabblight.glsl", Uniforms::VirtualResolution},
        {Shaders::Radiance, 0, "src/Shader/radiancelight.glsl"},
        {Shaders::Silhouette, 0, "src/Shader/silhouette.glsl"},
        {Shaders::Quantize, 0, "src/Shader/quantize.glsl", Uniforms::Palette},
        // {Shaders::ToneMap, 0, "src/Shader/toneMapping.glsl"},
        {Shaders::Glitch, 0, "src/Shader/glitch-ppEffect.glsl", Uniforms::Time | Uniforms::Resolution},
        {Shaders::ShadowLight, 0, "src/Shader/shadowLight.glsl", Uniforms::Time | Uniforms::VirtualResolution},
        {Shaders::Blur, 0, "src/Shader/blur.glsl", Uniforms::Resolution},
        {Shaders::BlurLowRes, 0, "src/Shader/blur.glsl", Uniforms::VirtualResolution},
        {Shaders::PostProcess, 0, "src/Shader/postProcess.glsl", Uniforms::VirtualResolution},
        {Shaders::EffectsMask, 0, "src/Shader/occlusionMask.glsl"},
        {Shaders::Test, 0, "src/Shader/test.glsl"},
    };

    constexpr s32 len = sizeof(shaderInfo) / sizeof(ShaderInfo);

    for (size_t i = 0; i < len; i++) {
        // dynamic allocation

#ifdef __EMSCRIPTEN__
        std::string vertexPath = shaderInfo[i].vertexPath ? std::string(shaderInfo[i].vertexPath) + ".web" : "";
        std::string fragPath = shaderInfo[i].fragPath ? std::string(shaderInfo[i].fragPath) + ".web" : "";
        const char* cVertexPath = vertexPath.empty() ? NULL : vertexPath.c_str();
        const char* cFragPath = fragPath.empty() ? NULL : fragPath.c_str();
        Shader shader = LoadShader(cVertexPath, cFragPath);
        if (cFragPath) {
            print("Loaded Shader: ", cFragPath);
        }
#else
        Shader shader = LoadShader(shaderInfo[i].vertexPath, shaderInfo[i].fragPath);
#endif
        s32 ix = static_cast<s32>(shaderInfo[i].shaderEnum);
        S_SHADERS[ix] = shader;
        setIsUsed(ix);

        S_UNIFORMS[ix].uniformFlags = shaderInfo[i].uniformFlags;

        if (S_UNIFORMS[ix].isSet(Uniforms::Time)) {
            S_UNIFORMS[ix].iTime = GetShaderLocation(shader, "iTime");
        }
        if (S_UNIFORMS[ix].isSet(Uniforms::Resolution) || S_UNIFORMS[ix].isSet(Uniforms::VirtualResolution)) {
            S_UNIFORMS[ix].iResolution = GetShaderLocation(shader, "iResolution");
        }

        // not working
        // if (S_UNIFORMS[ix].isSet(Uniforms::Palette)) {
        //     S_UNIFORMS[ix].iPalette = GetShaderLocation(shader, "iPalette");
        // }
    }
}

void ShaderManager::unloadAll() {
    const s32 maxShaderCount = static_cast<s32>(Shaders::_Count_DO_NOT_USE_ME);
    for (size_t i = 0; i < maxShaderCount; i++) {
        if (getIsUsed(i)) {
            UnloadShader(S_SHADERS[i]);
        }
    }
}

void ShaderManager::reloadShaders() {
    unloadAll();
    loadShaders();

    System::event.emit<ShaderReloadEvent>();
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
