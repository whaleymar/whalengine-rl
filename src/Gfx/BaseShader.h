#pragma once

#include <vector>
#include "Events/Events.h"
#include "Sys/IListen.h"
#include "Util/Types.h"
#include "Util/Vector.h"

namespace whal {

class BaseShader : public IListen<evt::ShaderReload, true> {
public:
    struct Uniform {
        std::string key;
        s32 handle;
    };

    BaseShader(const char* vsPath, const char* fsPath);
    virtual ~BaseShader();

    void onEvent(evt::ShaderReload) override;

    void setFloat(const char* name, f32 value);
    void setInt(const char* name, s32 value);
    void setTexture(const char* name, rl::Texture value);
    void setVector2(const char* name, rl::Vector2 value);
    void setVector2(const char* name, Vector2f value);
    void setVector3(const char* name, rl::Vector3 value);
    void setVector4(const char* name, rl::Vector4 value);

    s32 nameToId(const char* name) const;

    // Will check with OpenGL if not found
    s32 tryNameToId(const char* name);

    bool isValid() const { return mIsReady; }
    rl::Shader get() const { return mShaderHandle; }

protected:
    std::string mVertPath;
    std::string mFragPath;
    rl::Shader mShaderHandle;
    std::vector<Uniform> mNameToId;
    bool mIsReady;
};

class IShader {
public:
    // source and destination should not be the same RenderTexture. Use Graphics.getTemporaryRT if you need a temporary swap texture, or
    // gfx::applyShaders will do it for you automatically.
    virtual void process(rl::RenderTexture source, rl::RenderTexture destination) = 0;
    virtual ~IShader() {}
};

}  // namespace whal
