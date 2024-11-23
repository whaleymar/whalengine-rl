#include "BaseShader.h"

#include "Events/Events.h"
#include "raylib.h"

namespace whal {

static rl::Shader load(const std::string& vspath, const std::string& fspath) {
    const char* vsfinal = vspath == "" ? 0 : vspath.c_str();
    const char* fsfinal = fspath == "" ? 0 : fspath.c_str();
    return rl::LoadShader(vsfinal, fsfinal);
}

BaseShader::BaseShader(const char* vsPath, const char* fsPath) : mVertPath(vsPath), mFragPath(fsPath) {
    mShaderHandle = load(mVertPath, mFragPath);
    if (rl::IsShaderValid(mShaderHandle)) {
        mIsReady = true;
    } else {
        mIsReady = false;
    }
}

BaseShader::~BaseShader() {
    if (mIsReady) {
        rl::UnloadShader(mShaderHandle);
    }
}

void BaseShader::onEvent(evt::ShaderReload) {
    if (mIsReady) {
        rl::UnloadShader(mShaderHandle);
    }
    mShaderHandle = load(mVertPath, mFragPath);
    if (rl::IsShaderValid(mShaderHandle)) {
        mIsReady = true;
    } else {
        mIsReady = false;
        return;
    }

    // Reload uniforms
    std::vector<Uniform> oldUniforms = mNameToId;
    mNameToId.clear();
    for (auto [key, handle] : oldUniforms) {
        s32 newHandle = rl::GetShaderLocation(mShaderHandle, key.c_str());
        if (newHandle != -1) {
            mNameToId.push_back({key, newHandle});
        }
    }
}

void BaseShader::setFloat(const char* name, f32 value) {
    s32 handle = tryNameToId(name);
    assert(handle != -1);
    rl::SetShaderValue(mShaderHandle, handle, &value, rl::SHADER_UNIFORM_FLOAT);
}

void BaseShader::setInt(const char* name, s32 value) {
    s32 handle = tryNameToId(name);
    assert(handle != -1);
    rl::SetShaderValue(mShaderHandle, handle, &value, rl::SHADER_UNIFORM_INT);
}

void BaseShader::setTexture(const char* name, rl::Texture value) {
    s32 handle = tryNameToId(name);
    assert(handle != -1);
    rl::SetShaderValueTexture(mShaderHandle, handle, value);
}

void BaseShader::setVector2(const char* name, rl::Vector2 value) {
    s32 handle = tryNameToId(name);
    assert(handle != -1);
    rl::SetShaderValue(mShaderHandle, handle, &value, rl::SHADER_UNIFORM_VEC2);
}

void BaseShader::setVector2(const char* name, Vector2f value) {
    s32 handle = tryNameToId(name);
    assert(handle != -1);
    rl::SetShaderValue(mShaderHandle, handle, &value, rl::SHADER_UNIFORM_VEC2);
}

void BaseShader::setVector3(const char* name, rl::Vector3 value) {
    s32 handle = tryNameToId(name);
    assert(handle != -1);
    rl::SetShaderValue(mShaderHandle, handle, &value, rl::SHADER_UNIFORM_VEC3);
}

void BaseShader::setVector4(const char* name, rl::Vector4 value) {
    s32 handle = tryNameToId(name);
    assert(handle != -1);
    rl::SetShaderValue(mShaderHandle, handle, &value, rl::SHADER_UNIFORM_VEC4);
}

s32 BaseShader::nameToId(const char* name) const {
    for (auto [key, handle] : mNameToId) {
        if (isEqualString(key.c_str(), name)) {
            return handle;
        }
    }
    return -1;
}

s32 BaseShader::tryNameToId(const char* name) {
    s32 handle = nameToId(name);
    if (handle != -1) {
        return handle;
    }

    // not in table, try adding it
    handle = rl::GetShaderLocation(mShaderHandle, name);
    if (handle != -1) {
        mNameToId.push_back({name, handle});
    }
    return handle;
}

}  // namespace whal
