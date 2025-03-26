#include "Gfx/Texture.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <json.hpp>
#include <raylib.h>
#include <rlgl.h>
#include <string>

#include "Components/Animator.h"
#include "Gfx/RaylibUtil.h"
#include "Gfx/Shader.h"
#include "Map/AnimationFactory.h"
#include "Settings.h"

#include "Util/Print.h"
#include "Util/Vector.h"

#ifndef NDEBUG
#include "Gfx/ShaderManager.h"
#include "Sys/System.h"
#include "imgui.h"
#include "rfl/enums.hpp"
#endif

#define NULLOPT Corrade::Containers::NullOpt;

namespace whal {

static std::array<rl::RenderTexture2D, static_cast<s32>(TextureID::_COUNT_DO_NOT_USE_ME)> S_RENDER_TEXTURES;

Corrade::Containers::Optional<Error> TextureAtlas::init(const rl::Texture2D& texture, const std::string& atlasDataPath) {
    mTexture = texture;

    std::ifstream file(atlasDataPath);
    nlohmann::json doc;
    file >> doc;

    mIsTrimEnabled = doc["trim"];
    mIsRotateEnabled = doc["rotate"];

    if (!doc.contains("textures")) {
        return Error("Could not find 'textures'");
    }
    if (!doc["textures"].contains("atlas0")) {
        return Error("Could not find 'atlas0'");
    }

    const nlohmann::json& atlas = doc["textures"]["atlas0"];
    for (auto it = atlas.begin(); it != atlas.end(); ++it) {
        std::string name = it.key();
        s32 x = it.value().at("x");
        s32 y = it.value().at("y");
        s32 w = it.value().at("w");
        s32 h = it.value().at("h");

        // ignoring trim and rotate unless i need them
        rl::Rectangle frame = rl::Rectangle(x, y, w, h);
        mTable.insert({std::move(name), frame});
    }

    mIsValid = true;

    // add animations to the animation factory
    if (!doc.contains("animations")) {
        return Error("Could not find 'animations'");
    }

    const nlohmann::json& animations = doc["animations"];
    for (const nlohmann::json& anim : animations) {
        std::string name = anim["name"];
        const s32 frameCount = anim["framecount"];
        std::vector<Animation::FrameExt> frames;
        frames.reserve(frameCount);

        for (const nlohmann::json& frame : anim["frames"]) {
            f32 frameTime = frame["time"];
            s32 id = frame["id"];
            frames.push_back(Animation::FrameExt{
                .frame = *getFrame(whal_format("{}{}", name, id)),
                .duration = frameTime,
            });
        }

        AnimationFactory::add(std::move(name), Animation{
                                                   .frames = std::move(frames),
                                                   .name = name,
                                               });
    }

    return NULLOPT;
}

Vector2f TextureAtlas::getSize() const {
    return Vector2f(mTexture.width, mTexture.height);
}

Corrade::Containers::Optional<rl::Rectangle> TextureAtlas::getFrame(const std::string& name) const {
    auto search = mTable.find(name);
    if (search == mTable.end()) {
        return NULLOPT;
    }
    return search->second;
}

Corrade::Containers::Optional<rl::RenderTexture2D> TextureAtlas::frameToBackgroundTexture(const std::string& frameName) const {
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

MultiTexture MultiTexture::create(s32 width, s32 height, rl::PixelFormat format) {
    MultiTexture mt{
        .width = width,
        .height = height,
        .format = format,
    };

    // For WebGL, MRT targets need to be the same format and size
    mt.tex = rl::LoadRenderTextureFormat(width, height, format);
    rl::rlEnableFramebuffer(mt.tex.id);

    // Load additional buffers
    mt.depth = rl::rlLoadTexture(nullptr, width, height, format, 1);

    // Activate and attach the buffers
    rl::rlActiveDrawBuffers(2);
    rl::rlFramebufferAttach(mt.tex.id, mt.tex.texture.id, rl::RL_ATTACHMENT_COLOR_CHANNEL0, rl::RL_ATTACHMENT_TEXTURE2D, 0);
    rl::rlFramebufferAttach(mt.tex.id, mt.depth, rl::RL_ATTACHMENT_COLOR_CHANNEL1, rl::RL_ATTACHMENT_TEXTURE2D, 0);
    // RESEARCH add another buffer so opengl can do depth testing? could save some frames

    // Automatically calls rlDisableFramebuffer()
    if (!rl::rlFramebufferComplete(mt.tex.id)) {
        print("failed to create MultiTexture");
    }

    return mt;
}

void MultiTexture::release() {
    if (tex.id > 0) {
        // rl::rlUnloadTexture(tex.texture.id);
        rl::rlUnloadTexture(depth);
        // rl::rlUnloadFramebuffer(tex.id);
        rl::UnloadRenderTexture(tex);
        tex.id = 0;
    }
}

rl::Texture MultiTexture::getDepth() const {
    return rl::Texture{
        .id = depth,
        .width = width,
        .height = height,
        .mipmaps = 1,
        .format = format,
    };
}

enum class WindowSize {
    Game,
    Render,
    GlobalRange,
};

struct RenderTextureInfo {
    TextureID id;
    WindowSize size;
    rl::TextureFilter filter;
    rl::TextureWrap wrap;
    rl::PixelFormat format;
};

static const RenderTextureInfo S_RENDER_TEX_INFO[] = {
    {TextureID::Main, WindowSize::Render, rl::TEXTURE_FILTER_POINT, rl::TEXTURE_WRAP_REPEAT, rl::PIXELFORMAT_UNCOMPRESSED_R16G16B16A16},
    {TextureID::Lighting, WindowSize::Render, rl::TEXTURE_FILTER_BILINEAR, rl::TEXTURE_WRAP_REPEAT, rl::PIXELFORMAT_UNCOMPRESSED_R16G16B16A16},
    {TextureID::OcclusionColor, WindowSize::Game, rl::TEXTURE_FILTER_POINT, rl::TEXTURE_WRAP_REPEAT, rl::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8},
    {TextureID::Depth, WindowSize::Game, rl::TEXTURE_FILTER_POINT, rl::TEXTURE_WRAP_REPEAT, rl::PIXELFORMAT_UNCOMPRESSED_R8G8B8A8},
    {TextureID::Bloom, WindowSize::Render, rl::TEXTURE_FILTER_BILINEAR, rl::TEXTURE_WRAP_CLAMP, rl::PIXELFORMAT_UNCOMPRESSED_R16G16B16A16},
    {TextureID::DistanceField, WindowSize::GlobalRange, rl::TEXTURE_FILTER_POINT, rl::TEXTURE_WRAP_CLAMP, rl::PIXELFORMAT_UNCOMPRESSED_R16},
    {TextureID::OcclusionDepth, WindowSize::GlobalRange, rl::TEXTURE_FILTER_POINT, rl::TEXTURE_WRAP_CLAMP, rl::PIXELFORMAT_UNCOMPRESSED_R16},
};

TextureManager::TextureManager() {
    _loadRenderTextures();
}

TextureManager::~TextureManager() {
    if (!rl::IsWindowReady()) {
        return;
    }
    unloadAll();
    constexpr s32 rtLen = static_cast<s32>(TextureID::_COUNT_DO_NOT_USE_ME);
    for (size_t i = 0; i < rtLen; i++) {
        if (isRenderTextureUsed(i)) {
            rl::UnloadRenderTexture(S_RENDER_TEXTURES[i]);
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
    ImGui::Begin("Target Texture");
    ImGui::Combo("Texture", &selection, texNames.data(), texNames.size());
    ImGui::End();
}

void TextureManager::_setTargetTexture() {
    // do nothing if main is selected
    if (static_cast<TextureID>(selection) == TextureID::Main) {
        return;
    }
    rl::RenderTexture rt;
    if (!isRenderTextureUsed(selection)) {
        // invalid selection. Let's use this as an alias for gameobjects-only
        rt = Graphics.getStagingTex().tex;
    } else {
        // get the selection
        rt = getRenderTexture(static_cast<TextureID>(selection));
    }

    // ok, now set TextureID::Main to whatever we selected
    rl::RenderTexture mainTex = getRenderTexture(TextureID::Main);

    if (S_RENDER_TEX_INFO[selection].format == rl::PIXELFORMAT_UNCOMPRESSED_R16G16B16A16) {
        Graphics.blit(rt, mainTex, ShaderMgr::get("ToneMap").get());
    } else {
        Graphics.blit(rt, mainTex);
    }
}
#endif

Corrade::Containers::Optional<Error> TextureManager::registerTexture(const rl::Texture2D texture, const std::string& name) {
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

Corrade::Containers::Optional<Error> TextureManager::registerTextureAtlas(const rl::Texture2D texture, const std::string& atlasDataPath,
                                                                          const std::string& name) {
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

Corrade::Containers::Optional<Error> TextureManager::loadAndRegister(const std::string& imagePath, const std::string& name) {
    s32 ix = getTextureIndex(name);
    if (ix >= 0) {
        // already registered
        return NULLOPT;
    }

    rl::Texture2D texture = rl::LoadTexture(imagePath.c_str());
    if (!rl::IsTextureValid(texture)) {
        return Error(whal_format("Couldn't load image: %s", imagePath));
    }
    return registerTexture(texture, name);
}

Corrade::Containers::Optional<Error> TextureManager::loadAndRegisterAtlas(const std::string& imagePath, const std::string& atlasDataPath,
                                                                          const std::string& name) {
    rl::Texture2D texture = rl::LoadTexture(imagePath.c_str());
    if (!IsTextureValid(texture)) {
        return Error(whal_format("Couldn't load image: %s", imagePath));
    }
    return registerTextureAtlas(texture, atlasDataPath, name);
}

Corrade::Containers::Optional<Error> TextureManager::removeAtlas(const std::string& name) {
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
    rl::UnloadRenderTexture(_getRenderTexture(id));
    mRTUsageMask &= ~(1 << ix);
}

s32 TextureManager::getTextureIndex(const std::string& name) const {
    for (size_t i = 0; i < mTextureNames.size(); i++) {
        std::string texName = mTextureNames[i];
        if (texName == name) {
            return static_cast<s32>(i);
        }
    }
    return -1;
}

s32 TextureManager::getTextureAtlasIndex(const std::string& name) const {
    for (size_t i = 0; i < mTextureAtlasNames.size(); i++) {
        std::string texName = mTextureAtlasNames[i];
        if (texName == name) {
            return static_cast<s32>(i);
        }
    }
    return -1;
}

const TextureAtlas& TextureManager::_getAtlas(const std::string& name) {
    return mTextureAtlases[getTextureAtlasIndex(name)];
}

const rl::Texture2D& TextureManager::_getTexture(const std::string& name) {
    return mTextures[getTextureIndex(name)];
}

inline Vector2i getWindowSize(WindowSize size) {
    switch (size) {
    case WindowSize::Render:
        return {WINDOW_WIDTH_RENDER, WINDOW_HEIGHT_RENDER};
    case WindowSize::Game:
        return {WINDOW_WIDTH_GAME, WINDOW_HEIGHT_GAME};
    case WindowSize::GlobalRange:
        return {WINDOW_WIDTH_GAME * 2, WINDOW_HEIGHT_GAME * 2};
    }
}

void TextureManager::_loadRenderTextures() {
    for (auto rtInfo : S_RENDER_TEX_INFO) {
        Vector2i size = getWindowSize(rtInfo.size);
        rl::RenderTexture2D renderTexture = rl::LoadRenderTextureFormat(size.x, size.y, rtInfo.format);
        s32 ix = static_cast<s32>(rtInfo.id);
        S_RENDER_TEXTURES[ix] = renderTexture;
        setIsRenderTextureUsed(ix);
        rl::SetTextureFilter(renderTexture.texture, rtInfo.filter);

        if (rtInfo.wrap != rl::TEXTURE_WRAP_REPEAT) {
            rl::SetTextureWrap(renderTexture.texture, rtInfo.wrap);
        }
    }
}

void TextureManager::_unloadRenderTextures() {
    for (auto rtInfo : S_RENDER_TEX_INFO) {
        s32 ix = static_cast<s32>(rtInfo.id);
        if (isRenderTextureUsed(ix)) {
            unloadRenderTexture(rtInfo.id);
        }
    }
}

void TextureManager::unloadAll() {
    for (auto texture : getAllTextures()) {
        rl::UnloadTexture(texture);
    }
    for (auto atlas : getAllAtlases()) {
        rl::UnloadTexture(atlas.getTexture());
    }

    mTextureAtlases.clear();
    mTextureAtlasNames.clear();
    mTextures.clear();
    mTextureNames.clear();
}

void TextureManager::reloadRenderTextures() {
    _unloadRenderTextures();
    _loadRenderTextures();
}

}  // namespace whal
