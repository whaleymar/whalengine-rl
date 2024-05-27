#include "Gfx/Texture.h"

#include <cstdlib>
#include <cstring>
#include <format>
#include <raylib.h>
#include <string>

#include "ECS/Draw.h"
#include "ECS/Systems/TagTrackers.h"
#include "Settings.h"
#include "Util/FileUtils.h"
#include "Util/MathUtil.h"
#include "Util/Print.h"

#define RAPIDXML_NO_EXCEPTIONS
#include "RapidXML/rapidxml.hpp"

namespace rapidxml {

void parse_error_handler(const char* what, void* where) {
    print("Got XML parsing error: ", what);
    std::abort();
}

}  // namespace rapidxml

namespace whal {

Frame::Frame(Rectangle rect) : atlasPositionTexels(rect.x, rect.y), dimensionsTexels(rect.width, rect.height) {}

Frame::Frame(Vector2i atlasPosition, Vector2i dimensions) : atlasPositionTexels(atlasPosition), dimensionsTexels(dimensions) {}

std::optional<Error> TextureAtlas::init(const Texture2D& texture, const char* atlasDataPath) {
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

    return std::nullopt;
}

Vector2f TextureAtlas::getSize() const {
    return Vector2f(mTexture.width, mTexture.height);
}

std::optional<Rectangle> TextureAtlas::getFrame(const char* name) const {
    auto search = mTable.find(name);
    if (search == mTable.end()) {
        return std::nullopt;
    }
    return search->second;
}

std::optional<RenderTexture2D> TextureAtlas::frameToTexture(const char* frameName) const {
    std::optional<Rectangle> frameOpt = getFrame(frameName);
    if (!frameOpt) {
        return std::nullopt;
    }

    RenderTexture2D texture = LoadRenderTexture(WINDOW_WIDTH_PIXELS, WINDOW_HEIGHT_PIXELS);

    // want texture to align w/ bottom left of screen, so subtract height difference (since it defaults to top of screen)
    f32 heightDiff = WINDOW_HEIGHT_TEXELS - frameOpt->height;
    Rectangle dstRect = Rectangle(0, heightDiff * 2, frameOpt->width * FPIXELS_PER_TEXEL, frameOpt->height * FPIXELS_PER_TEXEL);
    BeginTextureMode(texture);

    ClearBackground(Colors::Clear);

    DrawTexturePro(getTexture(), frameOpt.value(), dstRect, {0.0f, 0.0f}, 0.0f, WHITE);

    EndTextureMode();

    const char* exportPath = "/home/whaley/code/whalengine-rl/tmpimg.png";
    Image img = LoadImageFromTexture(texture.texture);
    if (ExportImage(img, exportPath)) {
        print("wrote bg texture to ", exportPath);
    } else {
        print("error writing image");
    }

    return texture;
}

std::optional<Error> TextureManager::registerTexture(const Texture2D texture, const char* name) {
    s32 ix = getTextureIndex(name);
    if (ix >= 0) {
        print(std::format("Texture with name '{}' already registered", name), ". Replacing it.");
        UnloadTexture(mTextures[ix]);
        mTextures[ix] = texture;
    } else {
        mTextures.push_back(std::move(texture));
        mTextureNames.push_back(name);
    }
    return std::nullopt;
}

std::optional<Error> TextureManager::registerTextureAtlas(const Texture2D texture, const char* atlasDataPath, const char* name) {
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
    return std::nullopt;
}

std::optional<Error> TextureManager::loadAndRegister(const char* imagePath, const char* name) {
    Texture2D texture = LoadTexture(imagePath);
    if (!IsTextureReady(texture)) {
        return Error(std::format("Couldn't load image: %s", imagePath));
    }
    return registerTexture(texture, name);
}

std::optional<Error> TextureManager::loadAndRegisterAtlas(const char* imagePath, const char* atlasDataPath, const char* name) {
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

std::optional<Error> TextureManager::setBackgroundTextureToSprite(const char* atlasName, const char* spriteName, BGTexture dstBG, Vector2f parallax,
                                                                  Vector2i offset, bool isRepeatX, bool isRepeatY) {
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

    // ^ maybe i should check if the width/height is less than half the screen & then duplicate it
    // so if my frame is 100px wide then it gets drawn 3x
    // BUT that wouldn't tile neatly. Should only divide texture into powers of 2 (i.e. draw 1/2/4/8 frames evenly spaced)

    return std::nullopt;
}

void TextureManager::drawBackgroundTextures() {
    const Rectangle bgTextureDestRec = {0, 0, WINDOW_WIDTH_PIXELS, WINDOW_HEIGHT_PIXELS};
    Rectangle screenSourceRec;

    static Vector2i prevCameraPos = getCameraPosition();
    Vector2i cameraPos = getCameraPosition();
    Vector2f mvmt = toFloatVec(cameraPos - prevCameraPos);
    prevCameraPos = cameraPos;

    // check if we need to wrap
    // FAR
    mScrollFar -= (mvmt * mBGDataFar.parallax);
    s32 scrollSign;
    if (std::abs(mScrollFar.x()) >= mBGTextureFar->texture.width) {
        scrollSign = sign(mScrollFar.x());
        mScrollFar.e[0] = scrollSign * (std::abs(mScrollFar.x()) - mBGTextureFar->texture.width);
    }

    if (std::abs(mScrollFar.y()) >= mBGTextureFar->texture.height) {
        scrollSign = sign(mScrollFar.y());
        mScrollFar.e[1] = scrollSign * (std::abs(mScrollFar.y()) - mBGTextureFar->texture.height);
    }

    // MID
    mScrollMid -= (mvmt * mBGDataMid.parallax);
    if (std::abs(mScrollMid.x()) >= mBGTextureMid->texture.width) {
        scrollSign = sign(mScrollMid.x());
        mScrollMid.e[0] = scrollSign * (std::abs(mScrollMid.x()) - mBGTextureMid->texture.width);
    }

    if (std::abs(mScrollMid.y()) >= mBGTextureMid->texture.height) {
        scrollSign = sign(mScrollMid.y());
        mScrollMid.e[1] = scrollSign * (std::abs(mScrollMid.y()) - mBGTextureMid->texture.height);
    }

    // NEAR
    mScrollNear -= (mvmt * mBGDataNear.parallax);
    if (std::abs(mScrollNear.x()) >= mBGTextureNear->texture.width) {
        scrollSign = sign(mScrollNear.x());
        mScrollNear.e[0] = scrollSign * (std::abs(mScrollNear.x()) - mBGTextureNear->texture.width);
    }

    if (std::abs(mScrollNear.y()) >= mBGTextureNear->texture.height) {
        scrollSign = sign(mScrollNear.y());
        mScrollNear.e[1] = scrollSign * (std::abs(mScrollNear.y()) - mBGTextureNear->texture.height);
    }

    screenSourceRec = {0.0f, 0.0f, static_cast<f32>(mBGTextureStatic->texture.width), -1 * static_cast<f32>(mBGTextureStatic->texture.height)};
    auto drawBG = [bgTextureDestRec](Texture2D& texture, Rectangle screenSourceRec, Vector2f offset) -> void {
        DrawTexturePro(texture, screenSourceRec, {offset.x(), offset.y(), bgTextureDestRec.width, bgTextureDestRec.height}, {0.0f, 0.0f}, 0.0f,
                       WHITE);
    };

    drawBG(mBGTextureStatic->texture, screenSourceRec, {});

    // TODO if not repeating in X/Y, needs a root position
    // ALSo even if it does repeat, should have a root position so things look right

    // FAR BG:
    screenSourceRec = {0.0f, 0.0f, static_cast<f32>(mBGTextureFar->texture.width), -1 * static_cast<f32>(mBGTextureFar->texture.height)};

    drawBG(mBGTextureFar->texture, screenSourceRec, {mScrollFar.x(), -mScrollFar.y()});
    if (mBGDataFar.isRepeatX && mBGDataFar.isRepeatY) {
        // repeat right:
        drawBG(mBGTextureFar->texture, screenSourceRec, {bgTextureDestRec.width + mScrollFar.x(), -mScrollFar.y()});

        // repeat left:
        drawBG(mBGTextureFar->texture, screenSourceRec, {-bgTextureDestRec.width + mScrollFar.x(), -mScrollFar.y()});

        // repeat up:
        drawBG(mBGTextureFar->texture, screenSourceRec, {mScrollFar.x(), bgTextureDestRec.height - mScrollFar.y()});

        // repeat down:
        drawBG(mBGTextureFar->texture, screenSourceRec, {mScrollFar.x(), -bgTextureDestRec.height - mScrollFar.y()});

        // repeat top right
        drawBG(mBGTextureFar->texture, screenSourceRec, {bgTextureDestRec.width + mScrollFar.x(), bgTextureDestRec.height - mScrollFar.y()});

        // repeat top left
        drawBG(mBGTextureFar->texture, screenSourceRec, {-bgTextureDestRec.width + mScrollFar.x(), bgTextureDestRec.height - mScrollFar.y()});

        // repeat bottom right
        drawBG(mBGTextureFar->texture, screenSourceRec, {bgTextureDestRec.width + mScrollFar.x(), -bgTextureDestRec.height - mScrollFar.y()});

        // repeat bottom left
        drawBG(mBGTextureFar->texture, screenSourceRec, {-bgTextureDestRec.width + mScrollFar.x(), -bgTextureDestRec.height - mScrollFar.y()});

    } else if (mBGDataFar.isRepeatX) {
        // repeat right:
        drawBG(mBGTextureFar->texture, screenSourceRec, {bgTextureDestRec.width + mScrollFar.x(), -mScrollFar.y()});

        // repeat left:
        drawBG(mBGTextureFar->texture, screenSourceRec, {-bgTextureDestRec.width + mScrollFar.x(), -mScrollFar.y()});
    } else if (mBGDataFar.isRepeatY) {
        // repeat up:
        drawBG(mBGTextureFar->texture, screenSourceRec, {mScrollFar.x(), bgTextureDestRec.height - mScrollFar.y()});

        // repeat down:
        drawBG(mBGTextureFar->texture, screenSourceRec, {mScrollFar.x(), -bgTextureDestRec.height - mScrollFar.y()});
    }

    // MID BG:
    screenSourceRec = {0.0f, 0.0f, static_cast<f32>(mBGTextureMid->texture.width), -1 * static_cast<f32>(mBGTextureMid->texture.height)};

    drawBG(mBGTextureMid->texture, screenSourceRec, {mScrollMid.x(), -mScrollMid.y()});
    if (mBGDataMid.isRepeatX) {
        // repeat right:
        drawBG(mBGTextureMid->texture, screenSourceRec, {bgTextureDestRec.width + mScrollMid.x(), -mScrollMid.y()});

        // repeat left:
        drawBG(mBGTextureMid->texture, screenSourceRec, {-bgTextureDestRec.width + mScrollMid.x(), -mScrollMid.y()});
    }

    // NEAR BG:
    screenSourceRec = {0.0f, 0.0f, static_cast<f32>(mBGTextureNear->texture.width), -1 * static_cast<f32>(mBGTextureNear->texture.height)};

    drawBG(mBGTextureNear->texture, screenSourceRec, {mScrollNear.x(), -mScrollNear.y()});
    if (mBGDataNear.isRepeatX) {
        // repeat right:
        drawBG(mBGTextureNear->texture, screenSourceRec, {bgTextureDestRec.width + mScrollNear.x(), -mScrollNear.y()});

        // repeat left:
        drawBG(mBGTextureNear->texture, screenSourceRec, {-bgTextureDestRec.width + mScrollNear.x(), -mScrollNear.y()});
    }
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
