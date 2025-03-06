#include "ShaderManager.h"

#include <array>
#include <cassert>
#include <cstring>
#include <raylib.h>
#include "Gfx/ShaderTranspiler.h"
#include "Settings.h"
#include "Sys/System.h"

#include "Util/Print.h"

namespace whal {

struct Uniforms {
    enum flags : u32 {
        None = 0,
        TimeStamp = 1,
        Resolution = 1 << 1,
        VirtualResolution = 1 << 2,  // mutually exclusive w/ Resolution
    };

    bool isSet(flags flag) { return (uniformFlags & flag) > 0; }

    u32 uniformFlags = None;
    int iTime;
    int iResolution;
};

static std::array<rl::Shader, static_cast<s32>(Shaders::_Count_DO_NOT_USE_ME)> S_SHADERS;
static std::array<Uniforms, static_cast<s32>(Shaders::_Count_DO_NOT_USE_ME)> S_UNIFORMS;

static void ActivateShader(Shaders shaderEnum) {
    auto shader = ShaderManager::get(shaderEnum);
    s32 ix = static_cast<s32>(shaderEnum);
    Uniforms uniforms = S_UNIFORMS[ix];

    rl::BeginShaderMode(shader);

    if (uniforms.isSet(Uniforms::TimeStamp)) {
        const f32 iTime = Time.getElapsedUnmodified();
        rl::SetShaderValue(shader, uniforms.iTime, &iTime, rl::SHADER_UNIFORM_FLOAT);
    }

    if (uniforms.isSet(Uniforms::VirtualResolution)) {
        const f32 iResolution[2] = {FWINDOW_WIDTH_GAME, FWINDOW_HEIGHT_GAME};
        rl::SetShaderValue(shader, uniforms.iResolution, &iResolution, rl::SHADER_UNIFORM_VEC2);
    } else if (uniforms.isSet(Uniforms::Resolution)) {
        const f32 iResolution[2] = {FWINDOW_WIDTH_RENDER, FWINDOW_HEIGHT_RENDER};
        rl::SetShaderValue(shader, uniforms.iResolution, &iResolution, rl::SHADER_UNIFORM_VEC2);
    }
}

ScopedShader::ScopedShader(rl::Shader shader, bool isActivated) {
    if (!isActivated) {
        rl::BeginShaderMode(shader);
    }
}

ScopedShader::~ScopedShader() {
    rl::EndShaderMode();
}

void ShaderManager::loadShaders() {
    struct ShaderInfo {
        Shaders shaderEnum;
        const char* path;
        u32 uniformFlags = Uniforms::None;
    };

    static const ShaderInfo shaderInfo[] = {
        {Shaders::Default, "whalengine/src/Shader/DefaultSprite.glsl"},
        {Shaders::PointLight, "whalengine/src/Shader/PointLight.glsl"},
        {Shaders::BoxLight, "whalengine/src/Shader/AabbLight.glsl", Uniforms::VirtualResolution},
        {Shaders::ToneMap, "whalengine/src/Shader/ToneMapping.glsl"},
        {Shaders::ShadowLight, "whalengine/src/Shader/ShadowLight.glsl", Uniforms::TimeStamp | Uniforms::VirtualResolution},
        // {Shaders::Blur, "whalengine/src/Shader/Blur.glsl", Uniforms::Resolution},
        // {Shaders::BlurLowRes, "whalengine/src/Shader/Blur.glsl", Uniforms::VirtualResolution},
        // {Shaders::LightPassThru, "whalengine/src/Shader/LightPassThrough.glsl"},
        {Shaders::Overlay, "whalengine/src/Shader/TileOverlay.glsl"},
        {Shaders::Test, "whalengine/src/Shader/Test.glsl", Uniforms::Resolution | Uniforms::TimeStamp},
    };

    constexpr s32 len = sizeof(shaderInfo) / sizeof(ShaderInfo);
    ShaderTranspiler shaderTranspiler;

    for (size_t i = 0; i < len; i++) {
        // TESTING
        Expected<rl::Shader> eShader = shaderTranspiler.loadAndCompile(shaderInfo[i].path);
        if (!eShader.isExpected()) {
            print(eShader.error());
            continue;
        }
        s32 ix = static_cast<s32>(shaderInfo[i].shaderEnum);
        S_SHADERS[ix] = *eShader;
        setIsUsed(ix);

        S_UNIFORMS[ix].uniformFlags = shaderInfo[i].uniformFlags;

        if (S_UNIFORMS[ix].isSet(Uniforms::TimeStamp)) {
            S_UNIFORMS[ix].iTime = rl::GetShaderLocation(*eShader, "iTime");
        }
        if (S_UNIFORMS[ix].isSet(Uniforms::Resolution) || S_UNIFORMS[ix].isSet(Uniforms::VirtualResolution)) {
            S_UNIFORMS[ix].iResolution = rl::GetShaderLocation(*eShader, "iResolution");
        }
    }
}

void ShaderManager::unloadAll() {
    const s32 maxShaderCount = static_cast<s32>(Shaders::_Count_DO_NOT_USE_ME);
    for (size_t i = 0; i < maxShaderCount; i++) {
        if (getIsUsed(i)) {
            rl::UnloadShader(S_SHADERS[i]);
        }
    }
    mUsageMask = 0;
}

void ShaderManager::reloadShaders() {
    unloadAll();
    loadShaders();

    Event.emit<evt::ShaderReload>();
}

rl::Shader ShaderManager::get(Shaders shaderEnum) {
    return instance()._get(shaderEnum);
}

void ShaderManager::activate(Shaders shaderEnum) {
    ActivateShader(shaderEnum);
}

ScopedShader ShaderManager::activateScoped(Shaders shaderEnum) {
    ActivateShader(shaderEnum);
    return ScopedShader(instance()._get(shaderEnum), true);
}

rl::Shader ShaderManager::_get(Shaders shaderEnum) const {
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
