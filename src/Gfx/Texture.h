#pragma once

#include <raylib.h>

#include <optional>
#include <unordered_map>
#include <vector>

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

// static does not have parallax
enum class BGTexture { STATIC, FAR, MID, NEAR };

static const char* TEXNAME_BGSTATIC = "bgstatic";

class TextureAtlas {
public:
    std::optional<Error> init(const Texture2D& texture, const char* atlasDataPath);
    Vector2f getSize() const;
    std::optional<Rectangle> getFrame(const char* name) const;
    bool isValid() const { return mIsValid; }
    const Texture2D& getTexture() const { return mTexture; }
    std::optional<RenderTexture2D> frameToTexture(const char* frameName) const;

private:
    Texture2D mTexture;
    std::unordered_map<std::string, Rectangle> mTable;
    bool mIsTrimEnabled = false;
    bool mIsRotateEnabled = false;
    bool mIsValid = false;
};

class TextureManager {
public:
    static TextureManager& instance() {
        static TextureManager instance_;
        return instance_;
    }

    std::optional<Error> registerTexture(const Texture2D texture, const char* name);
    std::optional<Error> registerTextureAtlas(const Texture2D texture, const char* altasDataPath, const char* name);
    std::optional<Error> loadAndRegister(const char* imagePath, const char* name);
    std::optional<Error> loadAndRegisterAtlas(const char* imagePath, const char* atlasDataPath, const char* name);

    const Texture2D& getTexture(const char* name);
    const TextureAtlas& getTextureAtlas(const char* name);
    std::vector<Texture2D>& getAllTextures() { return mTextures; };
    std::vector<TextureAtlas>& getAllAtlases() { return mTextureAtlases; };

    // RESEARCH might need y position val?
    std::optional<Error> setBackgroundTextureToSprite(const char* atlasName, const char* spriteName, BGTexture dstBG, bool isRepeatVertical);
    void drawBackgroundTextures() const;

    void unloadAll();

private:
    TextureManager() = default;
    TextureManager(const TextureManager&) = delete;
    void operator=(const TextureManager&) = delete;

    s32 getTextureIndex(std::string name) const;
    s32 getTextureAtlasIndex(std::string name) const;

    std::vector<TextureAtlas> mTextureAtlases;
    std::vector<std::string> mTextureAtlasNames;
    std::vector<Texture2D> mTextures;
    std::vector<std::string> mTextureNames;

    // RenderTexture2D mBGTextureStatic;
    std::optional<RenderTexture2D> mBGTextureStatic;
    std::optional<RenderTexture2D> mBGTextureFar;
    std::optional<RenderTexture2D> mBGTextureMid;
    std::optional<RenderTexture2D> mBGTextureNear;
};

}  // namespace whal
