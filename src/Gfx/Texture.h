#pragma once

#include <raylib.h>

#include <optional>
#include <unordered_map>
#include <vector>

#include "Util/Vector.h"
#include "whalECS/src/Expected.h"

namespace whal {

struct Frame {
    Frame(Rectangle rect);
    Vector2i atlasPositionTexels;
    Vector2i dimensionsTexels;
};

static const char* TEXNAME_SPRITE = "sprite";

class TextureAtlas {
public:
    std::optional<Error> init(const Texture2D& texture, const char* atlasDataPath);
    Vector2f getSize() const;
    std::optional<Rectangle> getFrame(const char* name) const;
    bool isValid() const { return mIsValid; }
    const Texture2D& getTexture() { return mTexture; }

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
};

}  // namespace whal
