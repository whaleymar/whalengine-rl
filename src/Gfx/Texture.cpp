#include "Gfx/Texture.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <raylib.h>
#include <string>

#include "Components/Draw.h"
#include "Settings.h"
#include "Systems/TagTrackers.h"
#include "Util/FileUtils.h"
#include "Util/Print.h"
#include "Util/Vector.h"
#include "raylib/src/raylib.h"

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

Frame::Frame(Rectangle rect) : atlasPositionTexels(rect.x, rect.y), dimensionsTexels(rect.width, rect.height) {}

Frame::Frame(Vector2i atlasPosition, Vector2i dimensions) : atlasPositionTexels(atlasPosition), dimensionsTexels(dimensions) {}

Corrade::Containers::Optional<Error> TextureAtlas::init(const Texture2D& texture, const char* atlasDataPath) {
    mTexture = texture;
    Expected<std::string> content = readFile(atlasDataPath);
    if (!content.isExpected()) {
        return content.error();
    }

    using namespace rapidxml;

    xml_document<> doc;
    std::string xmldata = content.value();

    doc.parse<parse_no_entity_translation>(&xmldata[0]);
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

    s32 width = std::max(frameOpt->width, FWINDOW_WIDTH_TEXELS);
    s32 height = std::max(frameOpt->height, FWINDOW_HEIGHT_TEXELS);
    RenderTexture2D texture = LoadRenderTexture(width * PIXELS_PER_TEXEL, height * PIXELS_PER_TEXEL);

    // want texture to align w/ bottom left of screen, so subtract height difference (since it defaults to top of screen)
    f32 heightDiff = WINDOW_HEIGHT_TEXELS - frameOpt->height;
    Rectangle dstRect = Rectangle(0, heightDiff * 2, frameOpt->width * FPIXELS_PER_TEXEL, frameOpt->height * FPIXELS_PER_TEXEL);
    BeginTextureMode(texture);

    ClearBackground(Colors::Clear);

    DrawTexturePro(getTexture(), *frameOpt, dstRect, {0.0f, 0.0f}, 0.0f, WHITE);

    EndTextureMode();

    return texture;
}

TextureManager::TextureManager() {
    struct RenderTextureInfo {
        TextureID id;
        s32 width;
        s32 height;
        bool isUseBleedBuffer;
    };

    static const RenderTextureInfo sRenderTexInfo[] = {
        {TextureID::Main, WINDOW_WIDTH_PIXELS, WINDOW_HEIGHT_PIXELS, true},
        {TextureID::Background, WINDOW_WIDTH_PIXELS, WINDOW_HEIGHT_PIXELS, true},
        {TextureID::PostProcess, WINDOW_WIDTH_PIXELS, WINDOW_HEIGHT_PIXELS, true},
        {TextureID::Lighting, WINDOW_WIDTH_PIXELS, WINDOW_HEIGHT_PIXELS, true},
        {TextureID::Radiance, WINDOW_WIDTH_PIXELS, WINDOW_HEIGHT_PIXELS, true},
        {TextureID::LayerNormal, WINDOW_WIDTH_PIXELS, WINDOW_HEIGHT_PIXELS, true},
        {TextureID::LayerBloom, WINDOW_WIDTH_PIXELS, WINDOW_HEIGHT_PIXELS, true},
        {TextureID::LayerGlow, WINDOW_WIDTH_PIXELS, WINDOW_HEIGHT_PIXELS, true},
    };

    constexpr s32 len = sizeof(sRenderTexInfo) / sizeof(RenderTextureInfo);
    for (size_t i = 0; i < len; i++) {
        const auto rtInfo = sRenderTexInfo[i];
        s32 buffer = rtInfo.isUseBleedBuffer ? BLEED_SIZE : 0;
        RenderTexture2D renderTexture = LoadRenderTexture(rtInfo.width + buffer, rtInfo.height + buffer);
        s32 ix = static_cast<s32>(rtInfo.id);
        S_RENDER_TEXTURES[ix] = renderTexture;
        setIsRenderTextureUsed(ix);
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
    if (!IsTextureReady(texture)) {
        return Error(whal_format("Couldn't load image: %s", imagePath));
    }
    return registerTexture(texture, name);
}

Corrade::Containers::Optional<Error> TextureManager::loadAndRegisterAtlas(const char* imagePath, const char* atlasDataPath, const char* name) {
    Texture2D texture = LoadTexture(imagePath);
    if (!IsTextureReady(texture)) {
        return Error(whal_format("Couldn't load image: %s", imagePath));
    }
    return registerTextureAtlas(texture, atlasDataPath, name);
}

RenderTexture2D& TextureManager::_getRenderTexture(TextureID id) {
    s32 ix = static_cast<s32>(id);
    assert(isRenderTextureUsed(ix));
    return S_RENDER_TEXTURES[ix];
}

void TextureManager::setRenderTexture(TextureID id, RenderTexture2D rTexture) {
    s32 texIx = static_cast<s32>(TextureID::BackgroundStatic);
    if (isRenderTextureUsed(texIx)) {
        UnloadRenderTexture(_getRenderTexture(id));
    }
    S_RENDER_TEXTURES[texIx] = rTexture;
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

const Texture2D& TextureManager::getTexture(const char* name) {
    return mTextures[getTextureIndex(name)];
}

const TextureAtlas& TextureManager::getTextureAtlas(const char* name) {
    return mTextureAtlases[getTextureAtlasIndex(name)];
}

Corrade::Containers::Optional<Error> TextureManager::setBackgroundTextureToSprite(const char* atlasName, const char* spriteName, BGTexture dstBG,
                                                                                  Vector2f parallax, Vector2i offset, bool isRepeatX,
                                                                                  bool isRepeatY) {
    std::string texname;
    switch (dstBG) {
    case BGTexture::STATIC: {
        auto rTexOpt = getTextureAtlas(atlasName).frameToBackgroundTexture(spriteName);
        if (!rTexOpt) {
            return Error(whal_format("Couldn't create texture: {} is not in the {} atlas", spriteName, atlasName));
        }
        setRenderTexture(TextureID::BackgroundStatic, *rTexOpt);

        texname = "static";
        break;
    }

    case BGTexture::FAR: {
        auto rTexOpt = getTextureAtlas(atlasName).frameToBackgroundTexture(spriteName);
        if (!rTexOpt) {
            return Error(whal_format("Couldn't create texture: {} is not in the {} atlas", spriteName, atlasName));
        }
        setRenderTexture(TextureID::BackgroundFar, *rTexOpt);

        mScrollFar = {};
        texname = "far";
        mBGDataFar = {parallax, offset, isRepeatX, isRepeatY};
        break;
    }

    case BGTexture::MID: {
        auto rTexOpt = getTextureAtlas(atlasName).frameToBackgroundTexture(spriteName);
        if (!rTexOpt) {
            return Error(whal_format("Couldn't create texture: {} is not in the {} atlas", spriteName, atlasName));
        }
        setRenderTexture(TextureID::BackgroundMid, *rTexOpt);

        mScrollMid = {};
        texname = "mid";
        mBGDataMid = {parallax, offset, isRepeatX, isRepeatY};
        break;
    }

    case BGTexture::NEAR:
        auto rTexOpt = getTextureAtlas(atlasName).frameToBackgroundTexture(spriteName);
        if (!rTexOpt) {
            return Error(whal_format("Couldn't create texture: {} is not in the {} atlas", spriteName, atlasName));
        }
        setRenderTexture(TextureID::BackgroundNear, *rTexOpt);

        mScrollNear = {};
        texname = "near";
        mBGDataNear = {parallax, offset, isRepeatX, isRepeatY};
        break;
    }

    print("Loaded ", spriteName, " to texture", texname);

    // raylib: ""
    // NOTE: Be careful, background width must be equal or bigger than screen width
    // if not, texture should be draw more than two times for scrolling effect

    // ^ maybe i should check if the width/height is less than half the screen & then duplicate it
    // so if my frame is 100px wide then it gets drawn 3x
    // BUT that wouldn't tile neatly. Should only divide texture into powers of 2 (i.e. draw 1/2/4/8 frames evenly spaced)
    // OR i just leave it to the user to make it wide enough :)

    return NULLOPT;
}

void TextureManager::drawBackgroundTextures() {
    // currently, lighting does not affect this texture
    BeginTextureMode(getRenderTexture(TextureID::Background));
    ClearBackground(Colors::Clear);
    Rectangle screenSourceRec;

    // const Vector2f cameraPos = getCameraPositionPrecise();
    const Vector2f cameraPos = getCameraPosition().as<f32>();

    auto checkWrapping = [](const Vector2f cameraPos, const BGData bgdata, const s32 texWidth, const s32 texHeight, Vector2f& scrollVar) {
        f32 distance = cameraPos.x - (bgdata.worldPosTopLeftTexels.x * FPIXELS_PER_TEXEL);
        s32 offset = std::lerp<f32, f32>(texWidth, texWidth / 2, bgdata.parallax.x);
        s32 effectiveDistance = static_cast<s32>(std::round(distance * bgdata.parallax.x));
        if (bgdata.isRepeatX) {
            scrollVar.x = (texWidth - (effectiveDistance % texWidth) - offset) % texWidth;
        } else {
            scrollVar.x = texWidth - effectiveDistance - offset;
        }

        distance = cameraPos.y + texHeight - (bgdata.worldPosTopLeftTexels.y * FPIXELS_PER_TEXEL);
        offset = std::lerp<f32, f32>(texHeight, texHeight / 2, bgdata.parallax.y);
        effectiveDistance = static_cast<s32>(std::round(distance * bgdata.parallax.y));
        if (bgdata.isRepeatY) {
            scrollVar.y = (texHeight - (effectiveDistance % texHeight) - offset) % texHeight;
        } else {
            scrollVar.y = texHeight - effectiveDistance - offset;
        }
    };

    // check if we need to wrap
    if (isRenderTextureUsed(TextureID::BackgroundFar)) {
        auto& bgTex = getRenderTexture(TextureID::BackgroundFar);
        checkWrapping(cameraPos, mBGDataFar, bgTex.texture.width, bgTex.texture.height, mScrollFar);
    }
    if (isRenderTextureUsed(TextureID::BackgroundMid)) {
        auto& bgTex = getRenderTexture(TextureID::BackgroundMid);
        checkWrapping(cameraPos, mBGDataMid, bgTex.texture.width, bgTex.texture.height, mScrollMid);
    }
    if (isRenderTextureUsed(TextureID::BackgroundNear)) {
        auto& bgTex = getRenderTexture(TextureID::BackgroundNear);
        checkWrapping(cameraPos, mBGDataNear, bgTex.texture.width, bgTex.texture.height, mScrollNear);
    }

    auto drawBG = [](Texture2D& texture, Rectangle screenSourceRec, Vector2f offset, Color color = WHITE) -> void {
        DrawTexturePro(texture, screenSourceRec,
                       {offset.x - PIXELS_PER_TILE / 2, offset.y + PIXELS_PER_TILE / 2, (f32)texture.width, (f32)texture.height}, {0.0f, 0.0f}, 0.0f,
                       color);
    };

    auto drawBackgrounds = [drawBG](RenderTexture2D tex, Rectangle screenSourceRec, const BGData bgdata, Vector2f& scrollVar) {
        screenSourceRec = {0.0f, 0.0f, static_cast<f32>(tex.texture.width), -1 * static_cast<f32>(tex.texture.height)};

        drawBG(tex.texture, screenSourceRec, {scrollVar.x, -scrollVar.y});
        if (bgdata.isRepeatX && bgdata.isRepeatY) {
            // repeat right:
            drawBG(tex.texture, screenSourceRec, {tex.texture.width + scrollVar.x, -scrollVar.y});

            // repeat left:
            drawBG(tex.texture, screenSourceRec, {-tex.texture.width + scrollVar.x, -scrollVar.y});

            // repeat up:
            drawBG(tex.texture, screenSourceRec, {scrollVar.x, tex.texture.height - scrollVar.y});

            // repeat down:
            drawBG(tex.texture, screenSourceRec, {scrollVar.x, -tex.texture.height - scrollVar.y});

            // repeat top right
            drawBG(tex.texture, screenSourceRec, {tex.texture.width + scrollVar.x, tex.texture.height - scrollVar.y});

            // repeat top left
            drawBG(tex.texture, screenSourceRec, {-tex.texture.width + scrollVar.x, tex.texture.height - scrollVar.y});

            // repeat bottom right
            drawBG(tex.texture, screenSourceRec, {tex.texture.width + scrollVar.x, -tex.texture.height - scrollVar.y});

            // repeat bottom left
            drawBG(tex.texture, screenSourceRec, {-tex.texture.width + scrollVar.x, -tex.texture.height - scrollVar.y});

        } else if (bgdata.isRepeatX) {
            // repeat right:
            drawBG(tex.texture, screenSourceRec, {tex.texture.width + scrollVar.x, -scrollVar.y});

            // repeat left:
            drawBG(tex.texture, screenSourceRec, {-tex.texture.width + scrollVar.x, -scrollVar.y});
        } else if (bgdata.isRepeatY) {
            // repeat up:
            drawBG(tex.texture, screenSourceRec, {scrollVar.x, tex.texture.height - scrollVar.y});

            // repeat down:
            drawBG(tex.texture, screenSourceRec, {scrollVar.x, -tex.texture.height - scrollVar.y});
        }
    };

    // STATIC BG
    if (isRenderTextureUsed(TextureID::BackgroundStatic)) {
        auto& bgTex = getRenderTexture(TextureID::BackgroundStatic);
        screenSourceRec = {0.0f, 0.0f, static_cast<f32>(bgTex.texture.width), -1 * static_cast<f32>(bgTex.texture.height)};
        drawBG(bgTex.texture, screenSourceRec, {});
    }

    // FAR BG:
    if (isRenderTextureUsed(TextureID::BackgroundFar)) {
        auto& bgTex = getRenderTexture(TextureID::BackgroundFar);
        screenSourceRec = {0.0f, 0.0f, static_cast<f32>(bgTex.texture.width), -1 * static_cast<f32>(bgTex.texture.height)};
        drawBackgrounds(bgTex, screenSourceRec, mBGDataFar, mScrollFar);
    }

    // MID BG:
    if (isRenderTextureUsed(TextureID::BackgroundMid)) {
        auto& bgTex = getRenderTexture(TextureID::BackgroundMid);
        screenSourceRec = {0.0f, 0.0f, static_cast<f32>(bgTex.texture.width), -1 * static_cast<f32>(bgTex.texture.height)};
        drawBackgrounds(bgTex, screenSourceRec, mBGDataMid, mScrollMid);
    }

    // NEAR BG:
    if (isRenderTextureUsed(TextureID::BackgroundNear)) {
        auto& bgTex = getRenderTexture(TextureID::BackgroundNear);
        screenSourceRec = {0.0f, 0.0f, static_cast<f32>(bgTex.texture.width), -1 * static_cast<f32>(bgTex.texture.height)};
        drawBackgrounds(bgTex, screenSourceRec, mBGDataNear, mScrollNear);
    }

    EndTextureMode();
}

void TextureManager::drawLightingTexture() {
    RenderTexture2D lightingTexture = getRenderTexture(TextureID::Lighting);
    Rectangle screenSourceRec =
        Rectangle(0.0f, 0.0f, static_cast<f32>(lightingTexture.texture.width), -1 * static_cast<f32>(lightingTexture.texture.height));
    Rectangle dstRect(0, 0, lightingTexture.texture.width, lightingTexture.texture.height);

    BeginBlendMode(BLEND_MULTIPLIED);
    // doesnt look good, just makes everything look way brighter
    // BeginShaderMode(ShaderManager::get(Shaders::Bloom));
    DrawTexturePro(lightingTexture.texture, screenSourceRec, dstRect, {0.0f, 0.0f}, 0.0f, WHITE);
    // EndShaderMode();
    EndBlendMode();
}

void TextureManager::drawRadianceTexture() {
    // static int exposureUniform = GetShaderLocation(ShaderManager::get(Shaders::ToneMap), "exposure");
    // static float exposure = 1.0;
    //
    // if (IsKeyPressed(KEY_UP)) {
    //     exposure += 0.1;
    //     print("exposure: ", exposure);
    // } else if (IsKeyPressed(KEY_DOWN)) {
    //     exposure -= 0.1;
    //     print("exposure: ", exposure);
    // }

    BeginBlendMode(BLEND_ADDITIVE);
    RenderTexture2D radianceTexture = getRenderTexture(TextureID::Radiance);
    DrawTexture(radianceTexture.texture, 0, 0, WHITE);
    EndBlendMode();

    // auto shader = ShaderManager::get(Shaders::ToneMap);
    // BeginShaderMode(shader);
    // SetShaderValue(shader, exposureUniform, &exposure, SHADER_UNIFORM_FLOAT);
    // DrawTexture(getRenderTexture(TextureID::Main).texture, 0, 0, WHITE);
    // EndShaderMode();
}

void TextureManager::unloadAll() {
    for (auto texture : getAllTextures()) {
        UnloadTexture(texture);
    }
    for (auto atlas : getAllAtlases()) {
        UnloadTexture(atlas.getTexture());
    }

    constexpr s32 rtLen = static_cast<s32>(TextureID::_COUNT_DO_NOT_USE_ME);
    for (size_t i = 0; i < rtLen; i++) {
        if (isRenderTextureUsed(i)) {
            UnloadRenderTexture(S_RENDER_TEXTURES[i]);
        }
    }
}

}  // namespace whal
