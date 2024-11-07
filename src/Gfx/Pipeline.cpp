#include "Pipeline.h"

#include <raylib.h>

#include "Gfx/RaylibUtil.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"

#include "Util/Color.h"

namespace whal {

Pipeline::Pipeline(Pipeline&& other)
    : mShaders(std::move(other.mShaders)), mSwapBuffer(other.mSwapBuffer), mResolution(other.mResolution),
      mIsSwapBufferLoaded(other.mIsSwapBufferLoaded) {
    other.mIsSwapBufferLoaded = false;
}

Pipeline& Pipeline::operator=(Pipeline&& other) {
    if (this != &other) {
        if (mIsSwapBufferLoaded) {
            UnloadRenderTexture(mSwapBuffer);
        }
        mShaders = std::move(other.mShaders);
        mSwapBuffer = other.mSwapBuffer;
        mResolution = other.mResolution;
        mIsSwapBufferLoaded = other.mIsSwapBufferLoaded;

        other.mIsSwapBufferLoaded = false;
    }
    return *this;
}

Pipeline::Pipeline(Vector2i resolution, std::initializer_list<Shaders> shaders, bool isHDR)
    : mShaders(shaders), mResolution(resolution), mIsSwapBufferLoaded(true) {
    if (isHDR) {
        mSwapBuffer = LoadRenderTextureFormat(resolution.x, resolution.y, PIXELFORMAT_UNCOMPRESSED_R32G32B32A32);
    } else {
        mSwapBuffer = LoadRenderTexture(resolution.x, resolution.y);
    }
}

Pipeline::~Pipeline() {
    if (mIsSwapBufferLoaded) {
        UnloadRenderTexture(mSwapBuffer);
    }
}

void Pipeline::process(TextureID textureID) const {
    RenderTexture& processTexture = TextureManager::getRenderTexture(textureID);
    process(processTexture);
}

void Pipeline::process(RenderTexture2D& processTexture) const {
    assert(processTexture.texture.width == mResolution.x && processTexture.texture.height == mResolution.y &&
           "Pipeline resolution does not match passed RenderTexture");

    // initialize stuff for buffer swaps
    mTargetTexture = &processTexture;
    mActiveBuffer = mSwapBuffer;
    mIsDrawingToSwapBuffer = true;

    for (const auto shaderType : mShaders) {
        BeginTextureMode(mActiveBuffer);
        ShaderManager::activate(shaderType);
        swapBuffer();
        EndShaderMode();
        EndTextureMode();
    }

    if (!mIsDrawingToSwapBuffer) {
        // last draw was to mSwapBuffer, so need to update processTexture
        BeginTextureMode(processTexture);
        ClearBackground(Colors::CLEAR);
        gfx::DrawRenderTexture(mSwapBuffer);
        EndTextureMode();
    }
}

void Pipeline::swapBuffer() const {
    RenderTexture& processTexture = *mTargetTexture;

    ClearBackground(Colors::CLEAR);
    if (mIsDrawingToSwapBuffer) {
        gfx::DrawRenderTexture(processTexture);
    } else {
        gfx::DrawRenderTexture(mSwapBuffer);
    }

    mIsDrawingToSwapBuffer = !mIsDrawingToSwapBuffer;
    if (mIsDrawingToSwapBuffer) {
        mActiveBuffer = mSwapBuffer;
    } else {
        mActiveBuffer = processTexture;
    }
}

}  // namespace whal
