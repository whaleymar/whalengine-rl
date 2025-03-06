#pragma once

#include <vector>
#include "Events/Events.h"
#include "Sys/IListen.h"
#include "Util/Types.h"
#include "Util/Vector.h"

#ifndef NDEBUG
#include "Util/ImguiUtil.h"
#endif

namespace whal {

// TODO remove IListen from here. Only the ShaderManager should worry about reloads
class Shader : public IListen<evt::ShaderReload, true> {
public:
    struct Uniform {
        std::string key;
        s32 handle;
    };

    Shader(const char* unifiedShaderPath);
    virtual ~Shader();

    void onEvent(evt::ShaderReload) override;

    // TODO implement uniform caching. Basically store a vec of UniformVariants & each time a uniform is set, check if it's different from the cached
    // value. Only send the value to Renderer if it's different than the cached val.
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
    void invalidate() { mIsReady = false; }
    rl::Shader get() const { return mHandle; }

protected:
    std::string mShaderPath;
    rl::Shader mHandle;
    std::vector<Uniform> mNameToId;
    bool mIsReady;
};

class IShaderProcess
#ifndef NDEBUG
    : public IRenderDebug
#endif
{
public:
    // source and destination should not be the same RenderTexture. Use Graphics.getTemporaryRT if you need a temporary swap texture, or
    // gfx::applyShaders will do it for you automatically.
    virtual void process(rl::RenderTexture source, rl::RenderTexture destination) = 0;
    virtual ~IShaderProcess() {}
#ifndef NDEBUG
    void drawDebug() override {}
#endif
};

}  // namespace whal
