#include "Gfx/Texture.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <raylib.h>
#include <string>

#include "Gfx/RaylibUtil.h"
#include "Settings.h"

#include "Util/Color.h"
#include "Util/FileUtils.h"
#include "Util/Print.h"
#include "Util/Vector.h"

#define RAPIDXML_NO_EXCEPTIONS
#include "RapidXML/rapidxml.hpp"

namespace rapidxml {

void parse_error_handler(const char* what, void* where) {
    print("Got XML parsing error: ", what);
    std::abort();
}

}  // namespace rapidxml

#define NULLOPT Corrade::Containers::NullOpt;

namespace whal {

static std::array<RenderTexture2D, static_cast<s32>(TextureID::_COUNT_DO_NOT_USE_ME)> S_RENDER_TEXTURES;

Corrade::Containers::Optional<Error> TextureAtlas::init(const Texture2D& texture, const char* atlasDataPath) {
    using namespace rapidxml;

    mTexture = texture;

    xml_document<> doc;

    Expected<std::string> content = readFile(atlasDataPath);
    if (!content.isExpected()) {
        return content.error();
    }

    std::string xmldata = content.value();
    doc.parse<0>(&xmldata[0]);
    xml_node<>* atlasNode = doc.first_node("atlas");
    if (!atlasNode) {
        return Error("Could not find 'atlas' root node");
    }
    xml_node<>* trimNode = atlasNode->first_node("trim");
    if (!trimNode) {
        return Error("Could not find 'trim' node");
    }
    mIsTrimEnabled = strcmp(trimNode->value(), "true") == 0;

    xml_node<>* rotateNode = atlasNode->first_node("rotate");
    if (!rotateNode) {
        return Error("Could not find 'rotate' node");
    }
    mIsRotateEnabled = strcmp(rotateNode->value(), "true") == 0;

    xml_node<>* texNode = atlasNode->first_node("tex");
    if (!texNode) {
        return Error("Could not find 'tex' node");
    }

    for (xml_node<>* spriteNode = texNode->first_node("img"); spriteNode; spriteNode = spriteNode->next_sibling("img")) {
        const char* name = spriteNode->first_attribute("n")->value();
        Vector2i atlasPosition = {std::stoi(spriteNode->first_attribute("x")->value()), std::stoi(spriteNode->first_attribute("y")->value())};
        Vector2i dimensions = {std::stoi(spriteNode->first_attribute("w")->value()), std::stoi(spriteNode->first_attribute("h")->value())};

        // ignoring trim and rotate unless i need them
        Rectangle frame = Rectangle(atlasPosition.x, atlasPosition.y, dimensions.x, dimensions.y);
        mTable.insert({name, frame});
    }

    mIsValid = true;

    return NULLOPT;
}

Vector2f TextureAtlas::getSize() const {
    return Vector2f(mTexture.width, mTexture.height);
}

Corrade::Containers::Optional<Rectangle> TextureAtlas::getFrame(const char* name) const {
    auto search = mTable.find(name);
    if (search == mTable.end()) {
        return NULLOPT;
    }
    return search->second;
}

Corrade::Containers::Optional<RenderTexture2D> TextureAtlas::frameToBackgroundTexture(const char* frameName) const {
    Corrade::Containers::Optional<Rectangle> frameOpt = getFrame(frameName);
    if (!frameOpt) {
        return NULLOPT;
    }

    s32 width = std::max(frameOpt->width, FWINDOW_WIDTH_GAME);
    s32 height = std::max(frameOpt->height, FWINDOW_HEIGHT_GAME);
    RenderTexture2D texture = LoadRenderTexture(width, height);

    // want texture to align w/ bottom left of screen, so subtract height difference (since it defaults to top of screen)
    Rectangle dstRect = Rectangle(0, 0, frameOpt->width, frameOpt->height);

    BeginTextureMode(texture);
    ClearBackground(Colors::CLEAR);
    DrawTexturePro(getTexture(), *frameOpt, dstRect, {0.0f, 0.0f}, 0.0f, WHITE);
    EndTextureMode();

    return texture;
}

// RESEARCH add a data point for Texture Filter?
struct RenderTextureInfo {
    TextureID id;
    s32 width;
    s32 height;
    TextureFilter filter;
    bool isHDR;
};

static const RenderTextureInfo S_RENDER_TEX_INFO[] = {
    {TextureID::Main, WINDOW_WIDTH_RENDER, WINDOW_HEIGHT_RENDER, TEXTURE_FILTER_POINT, true},
    {TextureID::Lighting, WINDOW_WIDTH_RENDER, WINDOW_HEIGHT_RENDER, TEXTURE_FILTER_BILINEAR, true},
    {TextureID::OcclusionColor, WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME, TEXTURE_FILTER_POINT, false},
    {TextureID::OcclusionDepth, WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME, TEXTURE_FILTER_POINT, false},
    {TextureID::AllDepth, WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME, TEXTURE_FILTER_POINT, false},
    {TextureID::Bloom, WINDOW_WIDTH_RENDER, WINDOW_HEIGHT_RENDER, TEXTURE_FILTER_BILINEAR, true},
};

TextureManager::TextureManager() {
    for (auto rtInfo : S_RENDER_TEX_INFO) {
        auto format = rtInfo.isHDR ? PIXELFORMAT_UNCOMPRESSED_R16G16B16A16 : PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        RenderTexture2D renderTexture = LoadRenderTextureFormat(rtInfo.width, rtInfo.height, format);
        s32 ix = static_cast<s32>(rtInfo.id);
        S_RENDER_TEXTURES[ix] = renderTexture;
        setIsRenderTextureUsed(ix);
        SetTextureFilter(renderTexture.texture, rtInfo.filter);
    }
}

TextureManager::~TextureManager() {
    unloadAll();
    constexpr s32 rtLen = static_cast<s32>(TextureID::_COUNT_DO_NOT_USE_ME);
    for (size_t i = 0; i < rtLen; i++) {
        if (isRenderTextureUsed(i)) {
            UnloadRenderTexture(S_RENDER_TEXTURES[i]);
        }
    }
}

Corrade::Containers::Optional<Error> TextureManager::registerTexture(const Texture2D texture, const char* name) {
    s32 ix = getTextureIndex(name);
    if (ix >= 0) {
        print(whal_format("Texture with name '{}' already registered", name), ". Replacing it.");
        UnloadTexture(mTextures[ix]);
        mTextures[ix] = texture;
    } else {
        mTextures.push_back(std::move(texture));
        mTextureNames.push_back(name);
    }
    return NULLOPT;
}

Corrade::Containers::Optional<Error> TextureManager::registerTextureAtlas(const Texture2D texture, const char* atlasDataPath, const char* name) {
    s32 ix = getTextureAtlasIndex(name);
    if (ix >= 0) {
        return Error(whal_format("Texture Atlas with name '{}' already registered", name));
    }
    TextureAtlas atlas;
    auto errOpt = atlas.init(texture, atlasDataPath);
    if (errOpt) {
        return errOpt;
    }
    mTextureAtlases.push_back(atlas);
    mTextureAtlasNames.push_back(name);
    return NULLOPT;
}

Corrade::Containers::Optional<Error> TextureManager::loadAndRegister(const char* imagePath, const char* name) {
    Texture2D texture = LoadTexture(imagePath);
    if (!IsTextureValid(texture)) {
        return Error(whal_format("Couldn't load image: %s", imagePath));
    }
    return registerTexture(texture, name);
}

Corrade::Containers::Optional<Error> TextureManager::loadAndRegisterAtlas(const char* imagePath, const char* atlasDataPath, const char* name) {
    Texture2D texture = LoadTexture(imagePath);
    if (!IsTextureValid(texture)) {
        return Error(whal_format("Couldn't load image: %s", imagePath));
    }
    return registerTextureAtlas(texture, atlasDataPath, name);
}

Corrade::Containers::Optional<Error> TextureManager::removeAtlas(const char* name) {
    s32 ix = getTextureAtlasIndex(name);
    if (ix == -1) {
        return Error("Atlas not registered");
    }
    UnloadTexture(mTextureAtlases[ix].getTexture());
    mTextureAtlases.erase(mTextureAtlases.begin() + ix);
    mTextureAtlasNames.erase(mTextureAtlasNames.begin() + ix);
    return NULLOPT;
}

RenderTexture2D& TextureManager::_getRenderTexture(TextureID id) {
    s32 ix = static_cast<s32>(id);
    assert(isRenderTextureUsed(ix));
    return S_RENDER_TEXTURES[ix];
}

bool TextureManager::isRenderTextureUsed(s32 ix) const {
    return (mRTUsageMask & (1 << ix)) > 0;
}

void TextureManager::setIsRenderTextureUsed(s32 ix) {
    mRTUsageMask |= (1 << ix);
}

void TextureManager::unloadRenderTexture(TextureID id) {
    s32 ix = static_cast<s32>(id);
    assert(isRenderTextureUsed(ix) && "trying to unload RenderTexture that is already unloaded");
    UnloadRenderTexture(_getRenderTexture(id));
    mRTUsageMask &= ~(1 << ix);
}

s32 TextureManager::getTextureIndex(std::string name) const {
    for (size_t i = 0; i < mTextureNames.size(); i++) {
        std::string texName = mTextureNames[i];
        if (texName == name) {
            return static_cast<s32>(i);
        }
    }
    return -1;
}

s32 TextureManager::getTextureAtlasIndex(std::string name) const {
    for (size_t i = 0; i < mTextureAtlasNames.size(); i++) {
        std::string texName = mTextureAtlasNames[i];
        if (texName == name) {
            return static_cast<s32>(i);
        }
    }
    return -1;
}

const Texture2D& TextureManager::_getTexture(const char* name) {
    return mTextures[getTextureIndex(name)];
}

const TextureAtlas& TextureManager::_getAtlas(const char* name) {
    return mTextureAtlases[getTextureAtlasIndex(name)];
}

void TextureManager::unloadAll() {
    for (auto texture : getAllTextures()) {
        UnloadTexture(texture);
    }
    for (auto atlas : getAllAtlases()) {
        UnloadTexture(atlas.getTexture());
    }

    mTextureAtlases.clear();
    mTextureAtlasNames.clear();
    mTextures.clear();
    mTextureNames.clear();
}

}  // namespace whal
