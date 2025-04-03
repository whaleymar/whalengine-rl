#include "Shader.h"

#include "ShaderTranspiler.h"
#include "Sys/System.h"
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

Shader::Shader(rl::Shader loadedShader, const char* path) : mShaderPath(path) {
    if (rl::IsShaderValid(loadedShader)) {
        mHandle = loadedShader;
        mIsReady = true;
    } else {
        mIsReady = false;
    }
}

Shader::Shader(Shader&& other) {
    mShaderPath = std::move(other.mShaderPath);
    mHandle = other.mHandle;
    mNameToId = std::move(other.mNameToId);
    mIsReady = other.mIsReady;
    other.mIsReady = false;  // make sure destructor of other doesn't free shader
    other.mHandle.id = 0;    // doesn't really do anything but it's for my sanity
}

Shader::~Shader() {
    if (mIsReady && rl::IsWindowReady()) {
        Graphics.globalUniformOnShaderUnload(mHandle);
        rl::UnloadShader(mHandle);
    }
}

void Shader::bind() const {
    rl::BeginShaderMode(mHandle);
    Graphics.setUniforms(mHandle);
}

void Shader::unbind() const {
    rl::EndShaderMode();
}

void Shader::setFloat(const char* name, f32 value) {
    s32 loc = tryNameToId(name);
    // assert(handle != -1);
    Graphics.queueUniform(ShaderUniform{
        .value =
            UniformVariant{
                .tag = UniformVariant::Float,
                .val = {.uniFloat = value},
            },
        .loc = loc,
    });
}

void Shader::setInt(const char* name, s32 value) {
    s32 loc = tryNameToId(name);
    // assert(handle != -1);
    Graphics.queueUniform(ShaderUniform{
        .value =
            UniformVariant{
                .tag = UniformVariant::Int,
                .val = {.uniInt = value},
            },
        .loc = loc,
    });
}

void Shader::setTexture(const char* name, rl::Texture value) {
    s32 loc = tryNameToId(name);
    // assert(handle != -1);
    Graphics.queueUniform(ShaderUniform{
        .value =
            UniformVariant{
                .tag = UniformVariant::Texture,
                .val = {.uniTex = value.id},
            },
        .loc = loc,
    });
}

void Shader::setVector2(const char* name, rl::Vector2 value) {
    s32 loc = tryNameToId(name);
    // assert(handle != -1);
    Graphics.queueUniform(ShaderUniform{
        .value =
            UniformVariant{
                .tag = UniformVariant::Vec2,
                .val = {.uniVec2 = value},
            },
        .loc = loc,
    });
}

void Shader::setVector2(const char* name, Vector2f value) {
    s32 loc = tryNameToId(name);
    // assert(handle != -1);
    Graphics.queueUniform(ShaderUniform{
        .value =
            UniformVariant{
                .tag = UniformVariant::Vec2,
                .val = {.uniVec2 = value.asRL()},
            },
        .loc = loc,
    });
}

void Shader::setVector3(const char* name, rl::Vector3 value) {
    s32 loc = tryNameToId(name);
    // assert(handle != -1);
    Graphics.queueUniform(ShaderUniform{
        .value =
            UniformVariant{
                .tag = UniformVariant::Vec3,
                .val = {.uniVec3 = value},
            },
        .loc = loc,
    });
}

void Shader::setVector4(const char* name, rl::Vector4 value) {
    s32 loc = tryNameToId(name);
    // assert(handle != -1);
    Graphics.queueUniform(ShaderUniform{
        .value =
            UniformVariant{
                .tag = UniformVariant::Vec4,
                .val = {.uniVec4 = value},
            },
        .loc = loc,
    });
}

s32 Shader::nameToId(const char* name) const {
    for (const auto& [key, handle] : mNameToId) {
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
