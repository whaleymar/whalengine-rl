#pragma once

#include <raylib.h>
#include <vector>
#include "Util/Vector.h"

namespace whal {

enum class Shaders : s16;
enum class TextureID;

class Pipeline {
public:
    Pipeline() = default;
    Pipeline(const Pipeline&) = delete;
    Pipeline& operator=(const Pipeline&) = delete;
    Pipeline(Pipeline&& other);
    Pipeline& operator=(Pipeline&&);
    Pipeline(Vector2i resolution, std::initializer_list<Shaders> shaders);
    ~Pipeline();

    // applies the Pipeline's shader effects to the RenderTexture associated with the given ID. Overwrites the given texture!
    void process(TextureID textureID);
    void process(RenderTexture2D& renderTexture);

private:
    void swapBuffer();

    std::vector<Shaders> mShaders;
    RenderTexture2D mSwapBuffer;
    RenderTexture2D mActiveBuffer;
    RenderTexture2D* mTargetTexture = nullptr;
    Vector2i mResolution;
    bool mIsDrawingToSwapBuffer;
    bool mIsSwapBufferLoaded = false;
};

}  // namespace whal
