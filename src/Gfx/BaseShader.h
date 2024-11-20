#pragma once

#include <vector>
#include "Events/Events.h"
#include "Sys/IListen.h"
#include "Util/Types.h"
#include "Util/Vector.h"

namespace whal {

class BaseShader : public IListen<evt::ShaderReload, true> {
public:
    // RESEARCH if I add data type + value to this, I can expose these values to imgui
    struct Uniform {
        std::string key;
        s32 handle;
    };

    BaseShader(const char* vsPath, const char* fsPath);
    virtual ~BaseShader();

    virtual void process(RenderTexture source, RenderTexture destination) = 0;
    void onEvent(evt::ShaderReload) override;

    void setFloat(const char* name, f32 value);
    void setInt(const char* name, s32 value);
    void setTexture(const char* name, Texture value);
    void setVector2(const char* name, Vector2 value);
    void setVector2(const char* name, Vector2f value);
    void setVector3(const char* name, Vector3 value);
    void setVector4(const char* name, Vector4 value);

    s32 nameToId(const char* name) const;

    // Will check with OpenGL if not found
    s32 tryNameToId(const char* name);

    bool isValid() const { return mIsReady; }

protected:
    std::string mVertPath;
    std::string mFragPath;
    Shader mShaderHandle;
    std::vector<Uniform> mNameToId;
    bool mIsReady;
};

}  // namespace whal
