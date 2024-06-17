#include "Gfx/Texture.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <format>
#include <raylib.h>
#include <string>

#include "ECS/Draw.h"
#include "ECS/Systems/TagTrackers.h"
#include "Settings.h"
#include "Util/FileUtils.h"
#include "Util/Print.h"

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
        Rectangle frame = Rectangle(atlasPosition.x(), atlasPosition.y(), dimensions.x(), dimensions.y());
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

Corrade::Containers::Optional<RenderTexture2D> TextureAtlas::frameToTexture(const char* frameName) const {
    Corrade::Containers::Optional<Rectangle> frameOpt = getFrame(frameName);
    if (!frameOpt) {
        return NULLOPT;
    }

    RenderTexture2D texture = LoadRenderTexture(WINDOW_WIDTH_PIXELS, WINDOW_HEIGHT_PIXELS);

    // want texture to align w/ bottom left of screen, so subtract height difference (since it defaults to top of screen)
    f32 heightDiff = WINDOW_HEIGHT_TEXELS - frameOpt->height;
    Rectangle dstRect = Rectangle(0, heightDiff * 2, frameOpt->width * FPIXELS_PER_TEXEL, frameOpt->height * FPIXELS_PER_TEXEL);
    BeginTextureMode(texture);

    ClearBackground(Colors::Clear);

    DrawTexturePro(getTexture(), *frameOpt, dstRect, {0.0f, 0.0f}, 0.0f, WHITE);

    EndTextureMode();

    // debugging texture:
    // const char* exportPath = "/home/whaley/code/whalengine-rl/tmpimg.png";
    // Image img = LoadImageFromTexture(texture.texture);
    // if (ExportImage(img, exportPath)) {
    //     print("wrote bg texture to ", exportPath);
    // } else {
    //     print("error writing image");
    // }

    return texture;
}

TextureManager::TextureManager()
    : mLightingTexture(LoadRenderTexture(WINDOW_WIDTH_PIXELS, WINDOW_HEIGHT_PIXELS)), mBGTextureStatic(RenderTexture2D()),
      mBGTextureFar(RenderTexture2D()), mBGTextureMid(RenderTexture2D()), mBGTextureNear(RenderTexture2D()) {}

Corrade::Containers::Optional<Error> TextureManager::registerTexture(const Texture2D texture, const char* name) {
    s32 ix = getTextureIndex(name);
    if (ix >= 0) {
        print(std::format("Texture with name '{}' already registered", name), ". Replacing it.");
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
        return Error(std::format("Texture Atlas with name '{}' already registered", name));
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
        return Error(std::format("Couldn't load image: %s", imagePath));
    }
    return registerTexture(texture, name);
}

Corrade::Containers::Optional<Error> TextureManager::loadAndRegisterAtlas(const char* imagePath, const char* atlasDataPath, const char* name) {
    Texture2D texture = LoadTexture(imagePath);
    if (!IsTextureReady(texture)) {
        return Error(std::format("Couldn't load image: %s", imagePath));
    }
    return registerTextureAtlas(texture, atlasDataPath, name);
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
    case BGTexture::STATIC:
        if (mBGTextureStatic) {
            UnloadRenderTexture(*mBGTextureStatic);
        }
        mBGTextureStatic = getTextureAtlas(atlasName).frameToTexture(spriteName);
        if (!mBGTextureStatic) {
            return Error("Couldn't create texture");
        }
        texname = "static";
        break;

    case BGTexture::FAR:
        if (mBGTextureFar) {
            UnloadRenderTexture(*mBGTextureFar);
        }
        mBGTextureFar = getTextureAtlas(atlasName).frameToTexture(spriteName);
        if (!mBGTextureFar) {
            return Error("Couldn't create texture");
        }
        mScrollFar = {};
        texname = "far";
        mBGDataFar = {parallax, offset, isRepeatX, isRepeatY};
        break;

    case BGTexture::MID:
        if (mBGTextureMid) {
            UnloadRenderTexture(*mBGTextureMid);
        }
        mBGTextureMid = getTextureAtlas(atlasName).frameToTexture(spriteName);
        if (!mBGTextureMid) {
            return Error("Couldn't create texture");
        }
        mScrollMid = {};
        texname = "mid";
        mBGDataMid = {parallax, offset, isRepeatX, isRepeatY};
        break;

    case BGTexture::NEAR:
        if (mBGTextureNear) {
            UnloadRenderTexture(*mBGTextureNear);
        }
        mBGTextureNear = getTextureAtlas(atlasName).frameToTexture(spriteName);
        if (!mBGTextureNear) {
            return Error("Couldn't create texture");
        }
        mScrollNear = {};
        texname = "near";
        mBGDataNear = {parallax, offset, isRepeatX, isRepeatY};
        break;
    }

    print("Loaded ", spriteName, " to texture", texname);

    // raylib: ""
    // NOTE: Be careful, background width must be equal or bigger than screen width
    // if not, texture should be draw more than two times for scrolling effect
    // TODO i need a param for that ^
    // TODO what if the frame is wider than the window? probably broken

    // ^ maybe i should check if the width/height is less than half the screen & then duplicate it
    // so if my frame is 100px wide then it gets drawn 3x
    // BUT that wouldn't tile neatly. Should only divide texture into powers of 2 (i.e. draw 1/2/4/8 frames evenly spaced)

    return NULLOPT;
}

void TextureManager::drawBackgroundTextures() {
    const Rectangle bgTextureDestRec = {0, 0, WINDOW_WIDTH_PIXELS, WINDOW_HEIGHT_PIXELS};
    Rectangle screenSourceRec;

    const Vector2i cameraPos = getCameraPosition();

    auto checkWrapping = [](const Vector2i cameraPos, const BGData bgdata, const s32 texWidth, const s32 texHeight, Vector2f& scrollVar) {
        f32 distance = cameraPos.x() - (bgdata.worldPosTopLeftTexels.x() * FPIXELS_PER_TEXEL);
        s32 offset = std::lerp<f32, f32>(texWidth, texWidth / 2, bgdata.parallax.x());
        s32 effectiveDistance = static_cast<s32>(std::round(distance * bgdata.parallax.x()));
        if (bgdata.isRepeatX) {
            scrollVar.e[0] = (texWidth - (effectiveDistance % texWidth) - offset) % texWidth;
        } else {
            scrollVar.e[0] = texWidth - effectiveDistance - offset;
        }

        distance = cameraPos.y() + texHeight - (bgdata.worldPosTopLeftTexels.y() * FPIXELS_PER_TEXEL);
        offset = std::lerp<f32, f32>(texHeight, texHeight / 2, bgdata.parallax.y());
        effectiveDistance = static_cast<s32>(std::round(distance * bgdata.parallax.y()));
        if (bgdata.isRepeatY) {
            scrollVar.e[1] = (texHeight - (effectiveDistance % texHeight) - offset) % texHeight;
        } else {
            scrollVar.e[1] = texHeight - effectiveDistance - offset;
        }
    };

    // check if we need to wrap
    if (mBGTextureFar) {
        checkWrapping(cameraPos, mBGDataFar, mBGTextureFar->texture.width, mBGTextureFar->texture.height, mScrollFar);
    }
    if (mBGTextureMid) {
        checkWrapping(cameraPos, mBGDataMid, mBGTextureMid->texture.width, mBGTextureMid->texture.height, mScrollMid);
    }
    if (mBGTextureNear) {
        checkWrapping(cameraPos, mBGDataNear, mBGTextureNear->texture.width, mBGTextureNear->texture.height, mScrollNear);
    }

    screenSourceRec = {0.0f, 0.0f, static_cast<f32>(mBGTextureStatic->texture.width), -1 * static_cast<f32>(mBGTextureStatic->texture.height)};
    auto drawBG = [bgTextureDestRec](Texture2D& texture, Rectangle screenSourceRec, Vector2f offset, Color color = WHITE) -> void {
        DrawTexturePro(texture, screenSourceRec,
                       {offset.x() - PIXELS_PER_TILE / 2, offset.y() + PIXELS_PER_TILE / 2, bgTextureDestRec.width, bgTextureDestRec.height},
                       {0.0f, 0.0f}, 0.0f, color);
    };

    auto drawBackgrounds = [drawBG, bgTextureDestRec](RenderTexture2D tex, Rectangle screenSourceRec, const BGData bgdata, Vector2f& scrollVar) {
        screenSourceRec = {0.0f, 0.0f, static_cast<f32>(tex.texture.width), -1 * static_cast<f32>(tex.texture.height)};

        drawBG(tex.texture, screenSourceRec, {scrollVar.x(), -scrollVar.y()});
        if (bgdata.isRepeatX && bgdata.isRepeatY) {
            // repeat right:
            drawBG(tex.texture, screenSourceRec, {bgTextureDestRec.width + scrollVar.x(), -scrollVar.y()});

            // repeat left:
            drawBG(tex.texture, screenSourceRec, {-bgTextureDestRec.width + scrollVar.x(), -scrollVar.y()});

            // repeat up:
            drawBG(tex.texture, screenSourceRec, {scrollVar.x(), bgTextureDestRec.height - scrollVar.y()});

            // repeat down:
            drawBG(tex.texture, screenSourceRec, {scrollVar.x(), -bgTextureDestRec.height - scrollVar.y()});

            // repeat top right
            drawBG(tex.texture, screenSourceRec, {bgTextureDestRec.width + scrollVar.x(), bgTextureDestRec.height - scrollVar.y()});

            // repeat top left
            drawBG(tex.texture, screenSourceRec, {-bgTextureDestRec.width + scrollVar.x(), bgTextureDestRec.height - scrollVar.y()});

            // repeat bottom right
            drawBG(tex.texture, screenSourceRec, {bgTextureDestRec.width + scrollVar.x(), -bgTextureDestRec.height - scrollVar.y()});

            // repeat bottom left
            drawBG(tex.texture, screenSourceRec, {-bgTextureDestRec.width + scrollVar.x(), -bgTextureDestRec.height - scrollVar.y()});

        } else if (bgdata.isRepeatX) {
            // repeat right:
            drawBG(tex.texture, screenSourceRec, {bgTextureDestRec.width + scrollVar.x(), -scrollVar.y()});

            // repeat left:
            drawBG(tex.texture, screenSourceRec, {-bgTextureDestRec.width + scrollVar.x(), -scrollVar.y()});
        } else if (bgdata.isRepeatY) {
            // repeat up:
            drawBG(tex.texture, screenSourceRec, {scrollVar.x(), bgTextureDestRec.height - scrollVar.y()});

            // repeat down:
            drawBG(tex.texture, screenSourceRec, {scrollVar.x(), -bgTextureDestRec.height - scrollVar.y()});
        }
    };

    drawBG(mBGTextureStatic->texture, screenSourceRec, {});

    // FAR BG:
    screenSourceRec = {0.0f, 0.0f, static_cast<f32>(mBGTextureFar->texture.width), -1 * static_cast<f32>(mBGTextureFar->texture.height)};

    if (mBGTextureFar) {
        drawBackgrounds(*mBGTextureFar, screenSourceRec, mBGDataFar, mScrollFar);
    }

    // MID BG:
    screenSourceRec = {0.0f, 0.0f, static_cast<f32>(mBGTextureMid->texture.width), -1 * static_cast<f32>(mBGTextureMid->texture.height)};
    if (mBGTextureMid) {
        drawBackgrounds(*mBGTextureMid, screenSourceRec, mBGDataMid, mScrollMid);
    }

    // NEAR BG:
    screenSourceRec = {0.0f, 0.0f, static_cast<f32>(mBGTextureNear->texture.width), -1 * static_cast<f32>(mBGTextureNear->texture.height)};
    if (mBGTextureNear) {
        drawBackgrounds(*mBGTextureNear, screenSourceRec, mBGDataNear, mScrollNear);
    }
}

void TextureManager::drawLightingTexture() {
    Rectangle screenSourceRec =
        Rectangle(0.0f, 0.0f, static_cast<f32>(mLightingTexture.texture.width), -1 * static_cast<f32>(mLightingTexture.texture.height));
    Rectangle dstRect(0, 0, mLightingTexture.texture.width, mLightingTexture.texture.height);

    DrawTexturePro(mLightingTexture.texture, screenSourceRec, dstRect, {0.0f, 0.0f}, 0.0f, WHITE);
}

void TextureManager::unloadAll() {
    for (auto texture : getAllTextures()) {
        UnloadTexture(texture);
    }
    for (auto atlas : getAllAtlases()) {
        UnloadTexture(atlas.getTexture());
    }

    if (mBGTextureStatic) {
        UnloadRenderTexture(*mBGTextureStatic);
    }
    if (mBGTextureFar) {
        UnloadRenderTexture(*mBGTextureFar);
    }
    if (mBGTextureMid) {
        UnloadRenderTexture(*mBGTextureMid);
    }
    if (mBGTextureNear) {
        UnloadRenderTexture(*mBGTextureNear);
    }
}

}  // namespace whal
