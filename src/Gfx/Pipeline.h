#pragma once

#include <raylib.h>
#include "Util/Vector.h"

namespace whal {

enum class Shaders : s16;
enum class TextureID;

class Pipeline {
public:
    Pipeline(Vector2i resolution, std::initializer_list<Shaders> shaders);
    ~Pipeline();

    // applies the Pipeline's shader effects to the RenderTexture associated with the given ID. Overwrites the given texture!
    void process(TextureID textureID);

private:
    void swapBuffer();

    std::vector<Shaders> mShaders;
    RenderTexture2D mSwapBuffer;
    RenderTexture2D mActiveBuffer;
    TextureID mTargetTextureID;
    const Vector2i mResolution;
    bool mIsDrawingToSwapBuffer;
};

}  // namespace whal
