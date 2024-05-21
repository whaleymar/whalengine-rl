#include "Gfx/Texture.h"

#include <cstdlib>
#include <cstring>
#include <format>
#include <raylib.h>
#include <string>

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

namespace whal {

Frame::Frame(Rectangle rect) : atlasPositionTexels(rect.x, rect.y), dimensionsTexels(rect.width, rect.height) {}

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

std::optional<Error> TextureManager::registerTexture(const Texture2D texture, const char* name) {
    s32 ix = getTextureIndex(name);
    if (ix >= 0) {
        return Error(std::format("Texture with name '{}' already registered", name));
    }
    mTextures.push_back(std::move(texture));
    mTextureNames.push_back(name);
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

}  // namespace whal
