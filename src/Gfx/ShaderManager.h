#pragma once

#include "Util/Types.h"

typedef struct Shader Shader;

namespace whal {

enum class Shaders : s16 { Default = 0, PointLight, Radiance, Silhouette, Quantize, Outline, _Count_DO_NOT_USE_ME };

class ShaderManager {
public:
    static ShaderManager& instance() {
        static ShaderManager instance_;
        return instance_;
    }

    static Shader get(Shaders shaderEnum);

private:
    ShaderManager();
    ~ShaderManager();

    Shader _get(Shaders shaderEnum) const;
    void setIsUsed(s32 index);
    bool getIsUsed(s32 index) const;

    u32 mUsageMask = 0;
};

}  // namespace whal
