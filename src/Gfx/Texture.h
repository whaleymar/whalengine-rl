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

class TextureAtlas {
public:
    Corrade::Containers::Optional<Error> init(const rl::Texture2D& texture, const char* atlasDataPath);
    Vector2f getSize() const;
    Corrade::Containers::Optional<rl::Rectangle> getFrame(const char* name) const;
    bool isValid() const { return mIsValid; }
    const rl::Texture2D& getTexture() const { return mTexture; }
    Corrade::Containers::Optional<rl::RenderTexture2D> frameToBackgroundTexture(const char* frameName) const;

private:
    rl::Texture2D mTexture;
    std::unordered_map<std::string, rl::Rectangle> mTable;
    bool mIsTrimEnabled = false;
    bool mIsRotateEnabled = false;
    bool mIsValid = false;
};

// RESEARCH other LayerXYZs I might want to do in the future:
// - Outline
enum class TextureID {
    Main,
    Lighting,
    OcclusionColor,
    OcclusionDepth,
    AllDepth,
    Bloom,
    _COUNT_DO_NOT_USE_ME,
};

struct MultiTexture {
    rl::RenderTexture tex;
    u32 occlusionColor;
    u32 depth;           // stores depth for everything on red channel
    u32 occlusionDepth;  // stores depth for occluders on red channel

    rl::Texture getOcclusionColor() const;
    rl::Texture getDepth() const;
    rl::Texture getOcclusionDepth() const;
};

class TextureManager {
public:
    static TextureManager& instance() {
        static TextureManager instance_;
        return instance_;
    }

    // gets main render texture for drawing (TextureID::Main in non-debug builds)
#ifndef NDEBUG
    static void setTargetTexture() { instance()._setTargetTexture(); }
    static void drawTargetGui() { instance()._drawTargetGui(); }
    void _setTargetTexture();
    void _drawTargetGui();
    s32 selection = 0;
#endif

    Corrade::Containers::Optional<Error> registerTexture(const rl::Texture2D texture, const char* name);
    Corrade::Containers::Optional<Error> registerTextureAtlas(const rl::Texture2D texture, const char* altasDataPath, const char* name);
    Corrade::Containers::Optional<Error> loadAndRegister(const char* imagePath, const char* name);
    Corrade::Containers::Optional<Error> loadAndRegisterAtlas(const char* imagePath, const char* atlasDataPath, const char* name);
    Corrade::Containers::Optional<Error> removeAtlas(const char* name);

    static const TextureAtlas& getAtlas(const char* name) { return instance()._getAtlas(name); }
    static const rl::Texture& getTexture(const char* name) { return instance()._getTexture(name); }
    static rl::RenderTexture& getRenderTexture(TextureID id) { return instance()._getRenderTexture(id); }

    void unloadAll();

private:
    TextureManager();

    ~TextureManager();
    TextureManager(const TextureManager&) = delete;
    void operator=(const TextureManager&) = delete;

    s32 getTextureIndex(std::string name) const;
    s32 getTextureAtlasIndex(std::string name) const;
    std::vector<rl::Texture2D>& getAllTextures() { return mTextures; };
    std::vector<TextureAtlas>& getAllAtlases() { return mTextureAtlases; };
    const TextureAtlas& _getAtlas(const char* name);
    const rl::Texture2D& _getTexture(const char* name);
    rl::RenderTexture2D& _getRenderTexture(TextureID id);
    bool isRenderTextureUsed(s32 ix) const;
    void setIsRenderTextureUsed(s32 ix);
    void unloadRenderTexture(TextureID id);

    std::vector<TextureAtlas> mTextureAtlases;
    std::vector<std::string> mTextureAtlasNames;
    std::vector<rl::Texture2D> mTextures;
    std::vector<std::string> mTextureNames;

    u32 mRTUsageMask;
};

}  // namespace whal
