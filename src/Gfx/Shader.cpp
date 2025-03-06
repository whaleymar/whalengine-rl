#include "Shader.h"

#include "Events/Events.h"
#include "ShaderTranspiler.h"
#include "raylib.h"

#ifndef NDEBUG
#include "Util/Print.h"
#endif

namespace whal {

static Expected<rl::Shader> load(const std::string& path) {
    ShaderTranspiler shaderTranspiler;
    const char* finalPath = path == "" ? 0 : path.c_str();
    return shaderTranspiler.loadAndCompile(finalPath);
}

Shader::Shader(const char* unifiedShaderPath) : mShaderPath(unifiedShaderPath) {
    Expected<rl::Shader> eShader = load(mShaderPath);
    if (eShader.isExpected()) {
        mHandle = *eShader;
        mIsReady = true;
    } else {
#ifndef NDEBUG
        print(eShader.error());
#endif
        mIsReady = false;
    }
}

Shader::~Shader() {
    if (mIsReady && rl::IsWindowReady()) {
        rl::UnloadShader(mHandle);
    }
}

void Shader::onEvent(evt::ShaderReload) {
    if (mIsReady) {
        rl::UnloadShader(mHandle);
    }
    Expected<rl::Shader> eShader = load(mShaderPath);
    if (eShader.isExpected()) {
        mHandle = *eShader;
        mIsReady = true;
    } else {
#ifndef NDEBUG
        print(eShader.error());
#endif
        mIsReady = false;
        return;
    }

    // Reload uniforms
    std::vector<Uniform> oldUniforms = mNameToId;
    mNameToId.clear();
    for (auto [key, handle] : oldUniforms) {
        s32 newHandle = rl::GetShaderLocation(mHandle, key.c_str());
        if (newHandle != -1) {
            mNameToId.push_back({key, newHandle});
        }
    }
}

void Shader::setFloat(const char* name, f32 value) {
    s32 loc = tryNameToId(name);
    // assert(handle != -1);
    Graphics.queueUniform(UniformVariant{
        .tag = UniformVariant::UniformType::Float,
        .val = {.uniFloat = value},
        .uniformLoc = loc,
    });
}

void Shader::setInt(const char* name, s32 value) {
    s32 loc = tryNameToId(name);
    // assert(handle != -1);
    Graphics.queueUniform(UniformVariant{
        .tag = UniformVariant::UniformType::Int,
        .val = {.uniInt = value},
        .uniformLoc = loc,
    });
}

void Shader::setTexture(const char* name, rl::Texture value) {
    s32 loc = tryNameToId(name);
    // assert(handle != -1);
    Graphics.queueUniform(UniformVariant{
        .tag = UniformVariant::UniformType::Texture,
        .val = {.uniTex = value},
        .uniformLoc = loc,
    });
}

void Shader::setVector2(const char* name, rl::Vector2 value) {
    s32 loc = tryNameToId(name);
    // assert(handle != -1);
    Graphics.queueUniform(UniformVariant{
        .tag = UniformVariant::UniformType::Vec2,
        .val = {.uniVec2 = value},
        .uniformLoc = loc,
    });
}

void Shader::setVector2(const char* name, Vector2f value) {
    s32 loc = tryNameToId(name);
    // assert(handle != -1);
    Graphics.queueUniform(UniformVariant{
        .tag = UniformVariant::UniformType::Vec2,
        .val = {.uniVec2 = value.asRL()},
        .uniformLoc = loc,
    });
}

void Shader::setVector3(const char* name, rl::Vector3 value) {
    s32 loc = tryNameToId(name);
    // assert(handle != -1);
    Graphics.queueUniform(UniformVariant{
        .tag = UniformVariant::UniformType::Vec3,
        .val = {.uniVec3 = value},
        .uniformLoc = loc,
    });
}

void Shader::setVector4(const char* name, rl::Vector4 value) {
    s32 loc = tryNameToId(name);
    // assert(handle != -1);
    Graphics.queueUniform(UniformVariant{
        .tag = UniformVariant::UniformType::Vec4,
        .val = {.uniVec4 = value},
        .uniformLoc = loc,
    });
}

s32 Shader::nameToId(const char* name) const {
    for (auto [key, handle] : mNameToId) {
        if (isEqualString(key.c_str(), name)) {
            return handle;
        }
    }
    return -1;
}

s32 Shader::tryNameToId(const char* name) {
    s32 handle = nameToId(name);
    if (handle != -1) {
        return handle;
    }

    // not in table, try adding it
    handle = rl::GetShaderLocation(mHandle, name);
    if (handle != -1) {
        mNameToId.push_back({name, handle});
    }
    return handle;
}

}  // namespace whal
