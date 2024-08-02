#include "Pipeline.h"

#include <raylib.h>
#include "Components/Draw.h"
#include "Gfx/ShaderManager.h"
#include "Gfx/Texture.h"

namespace whal {

Pipeline::Pipeline(Vector2i resolution, std::initializer_list<Shaders> shaders)
    : mShaders(shaders), mSwapBuffer(LoadRenderTexture(resolution.x, resolution.y)), mResolution(resolution) {}

Pipeline::~Pipeline() {
    UnloadRenderTexture(mSwapBuffer);
}

static void drawTextureFlipped(const Texture& tex) {
    ClearBackground(Colors::Clear);
    DrawTextureRec(tex, Rectangle(0, 0, tex.width, -tex.height), Vector2(0, 0), WHITE);
}

void Pipeline::process(TextureID textureID) {
    RenderTexture& processTexture = TextureManager::getRenderTexture(textureID);
    process(processTexture);
}

void Pipeline::process(RenderTexture2D& processTexture) {
    assert(processTexture.texture.width == mResolution.x && processTexture.texture.height == mResolution.y &&
           "Pipeline resolution does not match passed RenderTexture");

    // initialize stuff for buffer swaps
    mTargetTexture = &processTexture;
    mActiveBuffer = mSwapBuffer;
    mIsDrawingToSwapBuffer = true;

    for (const auto shaderType : mShaders) {
        ScopedShader shaderScope = ShaderManager::activateScoped(shaderType);

        BeginTextureMode(mActiveBuffer);
        swapBuffer();
        EndTextureMode();
    }

    if (!mIsDrawingToSwapBuffer) {
        // last draw was to mSwapBuffer, so need to update processTexture
        BeginTextureMode(processTexture);
        drawTextureFlipped(mSwapBuffer.texture);
        EndTextureMode();
    }
}

void Pipeline::swapBuffer() {
    RenderTexture& processTexture = *mTargetTexture;

    if (mIsDrawingToSwapBuffer) {
        drawTextureFlipped(processTexture.texture);
    } else {
        drawTextureFlipped(mSwapBuffer.texture);
    }

    mIsDrawingToSwapBuffer = !mIsDrawingToSwapBuffer;
    if (mIsDrawingToSwapBuffer) {
        mActiveBuffer = mSwapBuffer;
    } else {
        mActiveBuffer = processTexture;
    }
}

}  // namespace whal
