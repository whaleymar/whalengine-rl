#include "Gfx/Texture.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <raylib.h>
#include <string>

#include "Gfx/RaylibUtil.h"
#include "Settings.h"

#include "Util/FileUtils.h"
#include "Util/Print.h"
#include "Util/Vector.h"

#define RAPIDXML_NO_EXCEPTIONS
#include "RapidXML/rapidxml.hpp"

#ifndef NDEBUG
#include "Gfx/ShaderManager.h"
#include "Sys/System.h"
#include "imgui.h"
#include "rfl/enums.hpp"
#endif

namespace rapidxml {

void parse_error_handler(const char* what, void* where) {
    print("Got XML parsing error: ", what);
    std::abort();
}

}  // namespace rapidxml

#define NULLOPT Corrade::Containers::NullOpt;

namespace whal {

static std::array<rl::RenderTexture2D, static_cast<s32>(TextureID::_COUNT_DO_NOT_USE_ME)> S_RENDER_TEXTURES;

Corrade::Containers::Optional<Error> TextureAtlas::init(const rl::Texture2D& texture, const char* atlasDataPath) {
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
        rl::Rectangle frame = rl::Rectangle(atlasPosition.x, atlasPosition.y, dimensions.x, dimensions.y);
        mTable.insert({name, frame});
    }

    mIsValid = true;

    return NULLOPT;
}

Vector2f TextureAtlas::getSize() const {
    return Vector2f(mTexture.width, mTexture.height);
}

Corrade::Containers::Optional<rl::Rectangle> TextureAtlas::getFrame(const char* name) const {
    auto search = mTable.find(name);
    if (search == mTable.end()) {
        return NULLOPT;
    }
    return search->second;
}

Corrade::Containers::Optional<rl::RenderTexture2D> TextureAtlas::frameToBackgroundTexture(const char* frameName) const {
    Corrade::Containers::Optional<rl::Rectangle> frameOpt = getFrame(frameName);
    if (!frameOpt) {
        return NULLOPT;
    }

    s32 width = std::max(frameOpt->width, FWINDOW_WIDTH_GAME);
    s32 height = std::max(frameOpt->height, FWINDOW_HEIGHT_GAME);
    rl::RenderTexture2D texture = rl::LoadRenderTexture(width, height);

    // want texture to align w/ bottom left of screen, so subtract height difference (since it defaults to top of screen)
    rl::Rectangle dstRect = rl::Rectangle(0, 0, frameOpt->width, frameOpt->height);

    rl::BeginTextureMode(texture);
    rl::ClearBackground(Colors::ClearRL);
    rl::DrawTexturePro(getTexture(), *frameOpt, dstRect, {0.0f, 0.0f}, 0.0f, rl::WHITE);
    rl::EndTextureMode();

    return texture;
}

rl::Texture MultiTexture::getOcclusionColor() const {
    return rl::Texture{
        .id = occlusionColor,
        .width = WINDOW_WIDTH_RENDER,
        .height = WINDOW_HEIGHT_RENDER,
        .mipmaps = 1,
        .format = rl::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };
}

rl::Texture MultiTexture::getDepth() const {
    return rl::Texture{
        .id = depth,
        .width = WINDOW_WIDTH_RENDER,
        .height = WINDOW_HEIGHT_RENDER,
        .mipmaps = 1,
        .format = rl::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };
}

rl::Texture MultiTexture::getOcclusionDepth() const {
    return rl::Texture{
        .id = occlusionDepth,
        .width = WINDOW_WIDTH_RENDER,
        .height = WINDOW_HEIGHT_RENDER,
        .mipmaps = 1,
        .format = rl::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    };
}

// RESEARCH add a data point for Texture Filter?
struct RenderTextureInfo {
    TextureID id;
    s32 width;
    s32 height;
    rl::TextureFilter filter;
    bool isHDR;
};

static const RenderTextureInfo S_RENDER_TEX_INFO[] = {
    {TextureID::Main, WINDOW_WIDTH_RENDER, WINDOW_HEIGHT_RENDER, rl::TEXTURE_FILTER_POINT, true},
    {TextureID::Lighting, WINDOW_WIDTH_RENDER, WINDOW_HEIGHT_RENDER, rl::TEXTURE_FILTER_BILINEAR, true},
    {TextureID::OcclusionColor, WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME, rl::TEXTURE_FILTER_POINT, false},
    {TextureID::OcclusionDepth, WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME, rl::TEXTURE_FILTER_POINT, false},
    {TextureID::AllDepth, WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME, rl::TEXTURE_FILTER_POINT, false},
    {TextureID::Bloom, WINDOW_WIDTH_RENDER, WINDOW_HEIGHT_RENDER, rl::TEXTURE_FILTER_BILINEAR, true},
    {TextureID::DistanceField, WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME, rl::TEXTURE_FILTER_POINT, false},
};

TextureManager::TextureManager() {
    for (auto rtInfo : S_RENDER_TEX_INFO) {
        auto format = rtInfo.isHDR ? rl::PIXELFORMAT_UNCOMPRESSED_R16G16B16A16 : rl::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        rl::RenderTexture2D renderTexture = LoadRenderTextureFormat(rtInfo.width, rtInfo.height, format);
        s32 ix = static_cast<s32>(rtInfo.id);
        S_RENDER_TEXTURES[ix] = renderTexture;
        setIsRenderTextureUsed(ix);
        SetTextureFilter(renderTexture.texture, rtInfo.filter);
    }
}

TextureManager::~TextureManager() {
    if (!rl::IsWindowReady()) {
        return;
    }
    unloadAll();
    constexpr s32 rtLen = static_cast<s32>(TextureID::_COUNT_DO_NOT_USE_ME);
    for (size_t i = 0; i < rtLen; i++) {
        if (isRenderTextureUsed(i)) {
            UnloadRenderTexture(S_RENDER_TEXTURES[i]);
        }
    }
}

#ifndef NDEBUG
static std::vector<const char*> getTextureNames() {
    std::vector<const char*> texNames;
    auto enumNames = rfl::get_enumerator_array<TextureID>();
    for (size_t i = 0; i < enumNames.size(); i++) {
        texNames.push_back(enumNames[i].first.data());
    }
    return texNames;
}
#endif

#ifndef NDEBUG
void TextureManager::_drawTargetGui() {
    static auto texNames = getTextureNames();
    // ImGui::TreeNode("Target Texture");
    // ImGui::BeginChild("Target Texture", ImVec2(-1, -1));
    ImGui::Begin("Target Texture");
    ImGui::Combo("Texture", &selection, texNames.data(), texNames.size());
    // ImGui::TreePop();
    // ImGui::EndChild();
    ImGui::End();
}

void TextureManager::_setTargetTexture() {
    // do nothing if invalid or default selection
    if (!isRenderTextureUsed(selection) || static_cast<TextureID>(selection) == TextureID::Main) {
        return;
    }

    // ok, now set TextureID::Main to whatever we selected
    rl::RenderTexture rt = getRenderTexture(static_cast<TextureID>(selection));
    rl::RenderTexture mainTex = getRenderTexture(TextureID::Main);

    if (S_RENDER_TEX_INFO[selection].isHDR) {
        Graphics.blit(rt, mainTex, ShaderManager::get(Shaders::ToneMap));
    } else {
        Graphics.blit(rt, mainTex);
    }
}
#endif

Corrade::Containers::Optional<Error> TextureManager::registerTexture(const rl::Texture2D texture, const char* name) {
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

Corrade::Containers::Optional<Error> TextureManager::registerTextureAtlas(const rl::Texture2D texture, const char* atlasDataPath, const char* name) {
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
    rl::Texture2D texture = rl::LoadTexture(imagePath);
    if (!IsTextureValid(texture)) {
        return Error(whal_format("Couldn't load image: %s", imagePath));
    }
    return registerTexture(texture, name);
}

Corrade::Containers::Optional<Error> TextureManager::loadAndRegisterAtlas(const char* imagePath, const char* atlasDataPath, const char* name) {
    rl::Texture2D texture = rl::LoadTexture(imagePath);
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

rl::RenderTexture2D& TextureManager::_getRenderTexture(TextureID id) {
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

const rl::Texture2D& TextureManager::_getTexture(const char* name) {
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
