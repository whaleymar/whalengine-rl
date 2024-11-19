#pragma once

#include <raylib.h>

#include <unordered_map>
#include <vector>

#include "CorradeOptional.h"

#include "Util/Vector.h"
#include "whalECS/src/Expected.h"

namespace whal {

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
    // Staging,
    Main,
    DownscaledPostProcess,
    Background,  // any repeating backgrounds use this
    Lighting,
    UpscaledLighting,
    Radiance,
    BackgroundStatic,
    BackgroundFar,
    BackgroundMid,
    BackgroundNear,
    OcclusionColor,
    OcclusionDepth,
    AllDepth,
    DownscaledBloom,
    UpscaledBloom,
    _COUNT_DO_NOT_USE_ME,
};

struct MultiTexture {
    RenderTexture tex;
    u32 occlusionColor;
    u32 depth;           // stores depth for everything on red channel
    u32 occlusionDepth;  // stores dpeth for occluders on red channel
};

class TextureManager {
    struct BGData {
        Vector2f parallax;
        Vector2i worldPosTopLeft;
        Vector2i trueDimensions;
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
    Corrade::Containers::Optional<Error> removeAtlas(const char* name);

    static const TextureAtlas& getAtlas(const char* name) { return instance()._getAtlas(name); }
    static const Texture& getTexture(const char* name) { return instance()._getTexture(name); }
    static RenderTexture& getRenderTexture(TextureID id) { return instance()._getRenderTexture(id); }
    static Vector2i getSize(TextureID id);

    Corrade::Containers::Optional<Error> setBackgroundTextureToSprite(const char* atlasName, const char* spriteName, BGTexture dstBG,
                                                                      Vector2f parallax, Vector2i offset, bool isRepeatX, bool isRepeatY);
    void renderBackgroundTextures();

    void unloadAll();

private:
    TextureManager();
    ~TextureManager();
    TextureManager(const TextureManager&) = delete;
    void operator=(const TextureManager&) = delete;

    s32 getTextureIndex(std::string name) const;
    s32 getTextureAtlasIndex(std::string name) const;
    std::vector<Texture2D>& getAllTextures() { return mTextures; };
    std::vector<TextureAtlas>& getAllAtlases() { return mTextureAtlases; };
    const TextureAtlas& _getAtlas(const char* name);
    const Texture2D& _getTexture(const char* name);
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
