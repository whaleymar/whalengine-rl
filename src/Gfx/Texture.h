#pragma once

#include <raylib.h>

#include <unordered_map>
#include <vector>

#include "CorradeOptional.h"

#include "Util/Vector.h"
#include "whalECS/src/Expected.h"

namespace whal {

struct Frame {
    Frame() = default;
    Frame(Rectangle rect);
    Frame(Vector2i, Vector2i);
    Vector2i atlasPositionTexels;
    Vector2i dimensionsTexels;
};

static const char* TEXNAME_SPRITE = "sprite";
static const char* TEXNAME_PALETTE = "palette";

// static does not have parallax
enum class BGTexture { STATIC, FAR, MID, NEAR };

static const char* TEXNAME_BGSTATIC = "bgstatic";

class TextureAtlas {
public:
    Corrade::Containers::Optional<Error> init(const Texture2D& texture, const char* atlasDataPath);
    Vector2f getSize() const;
    Corrade::Containers::Optional<Rectangle> getFrame(const char* name) const;
    bool isValid() const { return mIsValid; }
    const Texture2D& getTexture() const { return mTexture; }
    Corrade::Containers::Optional<RenderTexture2D> frameToBackgroundTexture(const char* frameName) const;

private:
    Texture2D mTexture;
    std::unordered_map<std::string, Rectangle> mTable;
    bool mIsTrimEnabled = false;
    bool mIsRotateEnabled = false;
    bool mIsValid = false;
};

// RESEARCH other LayerXYZs I might want to do in the future:
// - Outline
enum class TextureID {
    Main,
    Background,  // any repeating backgrounds use this
    PostProcess,
    // ColorGrade,  // this is a regular texture, not rendertexture, might remove
    Lighting,
    Radiance,
    LayerNormal,
    LayerBloom,
    LayerGlow,
    BackgroundStatic,
    BackgroundFar,
    BackgroundMid,
    BackgroundNear,
    Occlusion,
    _COUNT_DO_NOT_USE_ME,
};

class TextureManager {
    struct BGData {
        Vector2f parallax;
        Vector2i worldPosTopLeftTexels;
        bool isRepeatX;
        bool isRepeatY;
    };

public:
    static TextureManager& instance() {
        static TextureManager instance_;
        return instance_;
    }

    Corrade::Containers::Optional<Error> registerTexture(const Texture2D texture, const char* name);
    Corrade::Containers::Optional<Error> registerTextureAtlas(const Texture2D texture, const char* altasDataPath, const char* name);
    Corrade::Containers::Optional<Error> loadAndRegister(const char* imagePath, const char* name);
    Corrade::Containers::Optional<Error> loadAndRegisterAtlas(const char* imagePath, const char* atlasDataPath, const char* name);

    // TODO static getters
    const Texture2D& getTexture(const char* name);
    const TextureAtlas& getTextureAtlas(const char* name);
    std::vector<Texture2D>& getAllTextures() { return mTextures; };
    std::vector<TextureAtlas>& getAllAtlases() { return mTextureAtlases; };

    static RenderTexture& getRenderTexture(TextureID id) { return instance()._getRenderTexture(id); }

    Corrade::Containers::Optional<Error> setBackgroundTextureToSprite(const char* atlasName, const char* spriteName, BGTexture dstBG,
                                                                      Vector2f parallax, Vector2i offset, bool isRepeatX, bool isRepeatY);
    void drawBackgroundTextures();
    void drawLightingTexture();
    void drawRadianceTexture();

    void unloadAll();

private:
    TextureManager();
    TextureManager(const TextureManager&) = delete;
    void operator=(const TextureManager&) = delete;

    s32 getTextureIndex(std::string name) const;
    s32 getTextureAtlasIndex(std::string name) const;
    RenderTexture2D& _getRenderTexture(TextureID id);
    void setRenderTexture(TextureID id, RenderTexture2D rTexture);
    bool isRenderTextureUsed(s32 ix) const;
    bool isRenderTextureUsed(TextureID id) const { return isRenderTextureUsed(static_cast<s32>(id)); }
    void setIsRenderTextureUsed(s32 ix);
    void unloadRenderTexture(TextureID id);

    std::vector<TextureAtlas> mTextureAtlases;
    std::vector<std::string> mTextureAtlasNames;
    std::vector<Texture2D> mTextures;
    std::vector<std::string> mTextureNames;

    Vector2f mScrollFar;
    Vector2f mScrollMid;
    Vector2f mScrollNear;
    BGData mBGDataFar;
    BGData mBGDataMid;
    BGData mBGDataNear;

    u32 mRTUsageMask;
};

}  // namespace whal
