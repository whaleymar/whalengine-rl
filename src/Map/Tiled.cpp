#include "Tiled.h"

#include "Gfx/Texture.h"
#include "Map/EntityFactory.h"
#include "json.hpp"

#include "Settings.h"

// #include "Components/Draw.h"   // for background sprites
#include "Components/Light.h"  // for level ambient lighting
#include "Components/Name.h"
#include "Components/Transform.h"

#include "Gfx/Depth.h"
#include "Gfx/Frame.h"

#include "Map/ComponentFactory.h"
#include "Map/Level.h"
#include "Sys/System.h"
#include "Util/Print.h"
#include "Util/ResourceManager.h"
#include "rfl/enums.hpp"

#include <sstream>

#define NULLOPT Corrade::Containers::NullOpt;

namespace whal {

static const char* MAP_DIR = "data/map";

static ResourceManager<nlohmann::json, 50> S_TEMPLATE_MANAGER;
static ResourceManager<nlohmann::json, 250> S_MAP_MANAGER;

static TileSet parseTileset(const std::string& basename, s32 firstgid);
static void parseTileLayer(const nlohmann::json& layer, TileMap& map);
static void parseObjectLayer(const nlohmann::json& layer, ActiveLevel& level);
// static void parseImageLayer(const nlohmann::json& layer, ActiveLevel& level);
// static std::string getSpriteKeyFromPath(const std::string& spritePath);
static const nlohmann::json& getTemplate(std::string_view templateFile);
static const nlohmann::json& getMapFile(std::string_view mapFile);
static const nlohmann::json& getWorldFile(std::string_view mapFile);
static std::string getTypeFromTemplate(const std::string& templateFile);

void clearMapCache() {
    S_MAP_MANAGER.clearCache();
    S_TEMPLATE_MANAGER.clearCache();
}

Color parseColor(const std::string& hexString) {
    s32 r, g, b, a;
    // format is "#aarrggbb"
    std::istringstream(hexString.substr(1, 2)) >> std::hex >> a;
    std::istringstream(hexString.substr(3, 2)) >> std::hex >> r;
    std::istringstream(hexString.substr(5, 2)) >> std::hex >> g;
    std::istringstream(hexString.substr(7, 2)) >> std::hex >> b;

    return Color(r, g, b, a);
}

bool tryReadColor(const nlohmann::json& data, std::string_view key, Color* dst) {
    if (data.contains(key)) {
        std::string hexString = readString(data, key);
        *dst = parseColor(hexString);
        return true;
    }
    return false;
}

static void addComponents(ecs::Entity entity, EntityMapData entityData, const nlohmann::json& object, const nlohmann::json& allObjects,
                          const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, const ActiveLevel& level, LayerData layerData) {
    if (!object.contains("properties")) {
        return;
    }

    for (auto& property : object["properties"]) {
        std::string componentName = readString(property, "propertytype");
        ComponentAdder creatorFunc = nullptr;
        Prefab.component.getEntry(componentName.c_str(), &creatorFunc);
        if (creatorFunc == nullptr) {
            if (componentName == "InheritTemplate") {
                auto newTemplateFile = readString(property["value"], "TemplateFileName");
                std::string path = whal_format("templates/{}.tj", newTemplateFile);
                const auto& newPrefab = getTemplate(path);
                addComponents(entity, entityData, newPrefab, allObjects, idToIndex, level, layerData);
                EntityBuilder builderFunc = nullptr;
                Prefab.entity.getEntry(newTemplateFile.c_str(), &builderFunc);
                if (builderFunc != nullptr) {
                    builderFunc(entity, newPrefab, level);
                }
            }
            continue;
        }

        creatorFunc(property["value"], allObjects, idToIndex, entityData, level, entity, layerData);
    }
}

void TileSet::addTileComponents(ecs::Entity entity, s32 tileID, const ActiveLevel& level, const LayerData layerData, Vector2i mapPosition) const {
    const auto& object = getMapFile(fileName);
    bool isMissingTsetProps = false;

    if (!object.contains("properties")) {
        isMissingTsetProps = true;
    }

    const EntityMapData mapData = {mapPosition, {PIXELS_PER_TILE, PIXELS_PER_TILE}, 0, false, true};

    // don't need values for allObjects or idToIndex since tiles (should be) standalone entities
    const nlohmann::json emptyJson;
    const std::unordered_map<s32, std::pair<s32, ecs::Entity>> emptyIdToIndex;

    // add tileset components
    if (!isMissingTsetProps) {
        addComponents(entity, mapData, object, emptyJson, emptyIdToIndex, level, layerData);
    }

    // tile-specific component overrides
    s32 propsIx = tileIDToIndex[tileID];
    if (propsIx != -1) {
        addComponents(entity, mapData, object["tiles"][propsIx], emptyJson, emptyIdToIndex, level, layerData);
    }
}

TileMap TileMap::parse(const char* path, ActiveLevel& level) {
    const auto data = getMapFile(path);

    TileMap map;
    map.widthTiles = readInt(data, "width");
    map.heightTiles = readInt(data, "height");
    map.tileSize = readInt(data, "tilewidth");

    for (const auto& layer : data["layers"]) {
        bool isVisible = readBool(layer, "visible");
        if (!isVisible) {
            continue;
        }
        std::string type = readString(layer, "type");

        if (type == "tilelayer") {
            parseTileLayer(layer, map);
        } else if (type == "objectgroup") {
            parseObjectLayer(layer, level);
        } else if (type == "imagelayer") {
            // parseImageLayer(layer, level);
            print("parseImageLayer disabled!");
        } else {
            print("unrecognized layer: ", type, "\nSkipping for now");
        }
    }

    for (const auto& tileset : data["tilesets"]) {
        s32 firstgid = readInt(tileset, "firstgid");
        std::string fileName = readString(tileset, "source");

        TileSet tset = parseTileset(fileName, firstgid);
        map.tilesets.push_back(tset);
    }

    for (const auto& property : data["properties"]) {
        std::string propName = readString(property, "name");
        // std::string propType = readString(property, "type");
    }

    // add ambient lighting for the level
    auto eEntity = World.entity();
    if (eEntity.isExpected()) {
        auto lightEntity = eEntity.value();
        // idk why but i need 1 tile of extra height
        auto trans = Transform2D(level.worldOffset + (level.size * 0.5 + Vector2f(-FPIXELS_PER_TILE / 2, FPIXELS_PER_TILE)).as<s32>());
        trans.depth = Depth::Foreground2;
        lightEntity.add(trans);

        BoxLight boxLight = {{3 * PIXELS_PER_TILE, 0, level.lvlInfo.ambientLight}, (level.size * 0.5).as<s32>()};
        lightEntity.add(boxLight);

        level.childEntities.insert(lightEntity);
    } else {
        print("Couldn't allocate entity for level lighting");
    }

    return map;
}

static Depth getLayerDepth(nlohmann::json layer, Depth defaultDepth) {
    Depth layerDepth = defaultDepth;
    if (layer.contains("properties")) {
        for (auto& property : layer["properties"]) {
            std::string propertytype = readString(property, "propertytype");
            if (propertytype == "Depth") {
                layerDepth = parseDepth(property["value"]);
                break;
            }
        }
    }
    return layerDepth;
}

bool tryReadDepth(const nlohmann::json& data, std::string_view key, Depth* dst) {
    if (data.contains(key)) {
        std::string str = readString(data, key);
        *dst = parseDepth(str);
        return true;
    }
    return false;
}

Depth parseDepth(const std::string& depthString) {
    return rfl::string_to_enum<Depth>(depthString).value();
}

void parseTileLayer(const nlohmann::json& layer, TileMap& map) {
    s32 width = readInt(layer, "width");
    s32 height = readInt(layer, "height");
    const std::string name = readString(layer, "name");
    Depth layerDepth = getLayerDepth(layer, Depth::Level);

    TileLayer tLayer = {name, width, height, {layerDepth}, layer["data"].get<std::vector<s32>>()};
    map.layers.push_back(std::move(tLayer));
}

// this will create entities and immediately add them to the level
void parseObjectLayer(const nlohmann::json& layer, ActiveLevel& level) {
    using json = nlohmann::json;

    Depth layerDepth = getLayerDepth(layer, Depth::Level);
    LayerData layerData = {layerDepth};

    bool failedToAllocateEntities = false;
    const json& objects = layer["objects"];
    std::unordered_map<s32, std::pair<s32, ecs::Entity>> idToIndex;
    for (size_t ix = 0; ix < objects.size(); ix++) {
        s32 id = readInt(objects[ix], "id");

        auto eEntity = World.entity(false);
        if (!eEntity.isExpected()) {
            failedToAllocateEntities = true;
            break;
        }
        ecs::Entity entity = eEntity.value();
        level.childEntities.insert(entity);
        level.objects.push_back(entity);
        idToIndex.insert({id, {ix, entity}});
    }

    if (failedToAllocateEntities) {
        print("Failed to allocate entities for level ", level.filepath);

        // kill entities that we already made:
        for (auto [id, pair] : idToIndex) {
            pair.second.kill();
        }
        return;
    }

    for (const auto& object : objects) {
        bool isVisible = true;
        if (tryReadBool(object, "visible", &isVisible) && !isVisible) {
            continue;
        }
        std::string objType = "";
        bool isTypeFound = false;
        if (object.contains("type")) {
            objType = readString(object, "type");
            isTypeFound = true;
        } else if (object.contains("template")) {
            objType = getTypeFromTemplate(readString(object, "template"));
            if (objType.size()) {
                isTypeFound = true;
            }
        }

        if (!isTypeFound) {
            print("skipping object ID", readInt(object, "id"), "because it didn't have a type");
            continue;
        }
        if (objType != "Entity") {
            // check for metadata

            if (objType == "Map_CameraPoint") {
                Vector2i cameraPoint = readVector2i(object, "x", "y");
                level.cameraFocalPoint = getTransformFromMapPosition(cameraPoint, {0, 0}, level, true).position;
                // print("loaded camerapoint with pos", cameraPoint, "-->", level.cameraFocalPoint);

            } else if (objType == "Map_InitialSpawnPoint") {
                Vector2i spawnPoint = readVector2i(object, "x", "y");
                const Vector2i spawnPointWorldCoords = getTransformFromMapPosition(spawnPoint, {0, 0}, level, true).position;
                level.spawnPoints.push_back(spawnPointWorldCoords);
                level.initialSpawnPoint = spawnPointWorldCoords;
            } else {
                // print("Unrecognized object type: ", objType);
            }
            continue;
        }

        // check for prefab:
        const nlohmann::json* pPrefab = nullptr;
        if (object.contains("template")) {
            auto templateFile = readString(object, "template");
            const auto& prefab = getTemplate(templateFile);
            pPrefab = &prefab;
        }

        // now get transform
        // check position/size in prefab first, then object
        EntityMapData entityData;
        entityData.id = readInt(object, "id");
        ecs::Entity entity = idToIndex.at(entityData.id).second;
        bool hasPosition = false;
        entityData.isPoint = true;
        if (pPrefab) {
            hasPosition = tryReadVector2i(*pPrefab, "x", "y", &entityData.position);
        }
        hasPosition = tryReadVector2i(object, "x", "y", &entityData.position) || hasPosition;
        if (!hasPosition) {
            print("Entity with ID", entityData.id, "has no coordinates");
            entity.kill();
            continue;
        }
        if (pPrefab) {
            if (tryReadVector2i(*pPrefab, "width", "height", &entityData.size))
                entityData.isPoint = false;
        }
        if (tryReadVector2i(object, "width", "height", &entityData.size))
            entityData.isPoint = false;

        // add transform
        Transform2D trans = getTransformFromMapPosition(entityData.position, entityData.size, level, entityData.isPoint);
        trans.depth = layerData.depth;
        entity.add(trans);

        // add name
        std::string name = "";
        if (tryReadString(object, "name", &name)) {
            entity.add(Name(name.c_str()));
            print("created entity: ", name);
        }

        if (pPrefab) {
            // add template components
            entityData.isParsingTemplate = true;
            addComponents(entity, entityData, *pPrefab, objects, idToIndex, level, layerData);
            entityData.isParsingTemplate = false;

            // now run prefab factory function to do complicated stuff to components, like adding callbacks
            const auto prefabName = readString(*pPrefab, "name");
            EntityBuilder builderFunc = nullptr;
            Prefab.entity.getEntry(prefabName.c_str(), &builderFunc);
            if (builderFunc != nullptr) {
                builderFunc(entity, *pPrefab, level);
            }
        }

        // add object components with factory
        addComponents(entity, entityData, object, objects, idToIndex, level, layerData);
    }
}

// void parseImageLayer(const nlohmann::json& layer, ActiveLevel& level) {
//     Depth layerDepth = getLayerDepth(layer, Depth::Level);
//     LayerData layerData = {layerDepth};
//
//     Vector2i position = readVector2i(layer);
//
//     Vector2i offset;
//     tryReadVector2i(layer, "offsetx", "offsety", &offset);
//
//     position += offset + level.worldPosOrigin.as<s32>();
//
//     bool isRepeatX = false;
//     tryReadBool(layer, "repeatx", &isRepeatX);
//
//     bool isRepeatY = false;
//     tryReadBool(layer, "repeaty", &isRepeatY);
//
//     Vector2f parallax = {1.0, 1.0};
//     tryReadVector2f(layer, "parallaxx", "parallaxy", &parallax);
//
//     // std::string name = readString(layer, "name");
//     std::string imgPath = readString(layer, "image");
//     std::string spriteKey = getSpriteKeyFromPath(imgPath);
//
//     if (depthToFloat(layerDepth) < depthToFloat(Depth::Level)) {
//         // use background textures instead of an entity
//         BGTexture bgEnum;
//         switch (layerDepth) {
//         case Depth::BackgroundStatic:
//             bgEnum = BGTexture::STATIC;
//             break;
//
//         case Depth::BackgroundFar:
//             bgEnum = BGTexture::FAR;
//             break;
//
//         case Depth::BackgroundMid:
//             bgEnum = BGTexture::MID;
//             break;
//
//         case Depth::BackgroundNear:
//             bgEnum = BGTexture::NEAR;
//             break;
//
//         default:
//             print("Found Depth enum value which doesn't match one of {Static, Far, Mid, Near}. Defeaulting to Mid");
//             bgEnum = BGTexture::MID;
//         }
//
//         auto errOpt = TextureManager::instance().setBackgroundTextureToSprite(TEXNAME_SPRITE, spriteKey.c_str(), bgEnum, parallax, position,
//                                                                               isRepeatX, isRepeatY);
//         if (errOpt) {
//             print("Got error: ", *errOpt);
//         }
//
//         return;
//     }
//
//     Corrade::Containers::Optional<Rectangle> frameOpt = TextureManager::getAtlas(TEXNAME_SPRITE).getFrame(spriteKey.c_str());
//     if (!frameOpt) {
//         return;
//     }
//
//     auto eEntity = World.entity();
//     if (!eEntity.isExpected()) {
//         return;
//     }
//
//     Frame frame(*frameOpt);
//     ecs::Entity entity = eEntity.value();
//     level.childEntities.insert(entity);
//
//     Transform2D trans = getTransformFromMapPosition(position + offset, frame.size, level, false);
//     entity.add(trans);
//
//     entity.add(Sprite(layerData.depth, frame));
// }

TileSet parseTileset(const std::string& basename, s32 firstgid) {
    const auto data = getMapFile(basename);

    auto sourceFilePath = readString(data, "image");

    s32 firstIx = std::max<s32>(sourceFilePath.find_last_of('/'), sourceFilePath.find_last_of('\\')) + 1;
    s32 lastIx = sourceFilePath.find(".", firstIx);
    std::string sourceFileBasenameNoExt = sourceFilePath.substr(firstIx, lastIx - firstIx);

    s32 tileWidth = readInt(data, "tilewidth");
    s32 tileHeight = readInt(data, "tileheight");
    s32 width = readInt(data, "imagewidth");
    s32 height = readInt(data, "imageheight");

    s32 widthTiles = width / tileWidth;
    s32 heightTiles = height / tileHeight;
    s32 tilecount = readInt(data, "tilecount");

    std::vector<s32> idToIx(tilecount, -1);
    if (data.contains("tiles")) {
        s32 ix = 0;
        for (const auto& tiledata : data["tiles"]) {
            s32 id = readInt(tiledata, "id");
            idToIx[id] = ix++;
        }
    }

    return TileSet(firstgid, tilecount, tileWidth, tileHeight, widthTiles, heightTiles, data["margin"], data["spacing"], basename,
                   sourceFileBasenameNoExt, std::move(idToIx));
}

const TileSet& getTileSet(const TileMap& map, s32 blockId) {
    for (size_t i = 0; i < map.tilesets.size(); i++) {
        s32 firstgid = map.tilesets[i].firstgid;
        if (firstgid <= blockId && blockId < firstgid + map.tilesets[i].tilecount) {
            return map.tilesets[i];
        }
    }
    print("error getting tileset for blockIx", blockId, "\nReturning first tileset instead");
    return map.tilesets[0];
}

Expected<Frame> getTileFrame(const TileMap& map, s32 blockId) {
    const TileSet& tset = getTileSet(map, blockId);
    std::string spritePath = whal_format("{}/{}", "map", tset.spriteFileName);
    Corrade::Containers::Optional<Rectangle> tsetFrameOpt = TextureManager::getAtlas(TEXNAME_SPRITE).getFrame(spritePath.c_str());

    if (!tsetFrameOpt) {
        return Error(whal_format("Couldn't find {} in sprite table", spritePath));
    }

    // ASSUMING 0 MARGIN && SPACING

    s32 blockIx = blockId - tset.firstgid;

    s32 rowIx = blockIx / tset.widthTiles;
    s32 colIx = blockIx % tset.widthTiles;

    Frame fullFrame = *tsetFrameOpt;
    Frame newFrame = {{fullFrame.atlasPosition.x + colIx * tset.tileWidth, fullFrame.atlasPosition.y + rowIx * tset.tileHeight},
                      {tset.tileWidth, tset.tileHeight}};
    return newFrame;
}

void parseMapProject(const char* mapfile) {
    const auto data = getWorldFile(mapfile);
    for (auto& propType : data["propertyTypes"]) {
        Prefab.component.makeDefaultComponent(propType);
    }
}

// parses a level's parameters and returns its LevelInfo struct
static Expected<Level::LevelInfo> parseLevelInfo(const char* lvlFileName) {
    const auto data = getMapFile(lvlFileName);
    for (auto& property : data["properties"]) {
        std::string propType = readString(property, "propertytype");
        if (propType == "Map_MapInfo") {
            auto mapInfo = property["value"];
            Level::LevelInfo lvlInfo;
            tryReadColor(mapInfo, "AmbientLight", &lvlInfo.ambientLight);
            tryReadBool(mapInfo, "isWorldEntryPoint", &lvlInfo.isWorldEntryPoint);
            return lvlInfo;
        }
    }

    return Error(whal_format("LevelInfo property not found in level: {}", lvlFileName));
}

Corrade::Containers::Optional<Error> parseWorld(const char* mapfile, Scene& dstScene) {
    const auto data = getWorldFile(mapfile);

#ifndef NDEBUG
    std::string type = readString(data, "type");
    if (type != "world") {
        return Error("Not a world file");
    }
#endif

    dstScene.name = mapfile;
    for (auto& map : data["maps"]) {
        std::string filename = map["fileName"];
        s32 x = readInt(map, "x");
        s32 y = readInt(map, "y");
        s32 width = readInt(map, "width");
        s32 height = readInt(map, "height");
        Expected<Level::LevelInfo> eLvlInfo = parseLevelInfo(filename.c_str());
        if (eLvlInfo.isExpected()) {
            Level lvl = {filename, Vector2f(x, -y), Vector2f(width, height), eLvlInfo.value()};
            if (lvl.lvlInfo.isWorldEntryPoint) {
                auto errOpt = dstScene.setStartLevelIx(dstScene.allLevels.size());
                if (errOpt) {
                    return *errOpt;
                }
            }

            dstScene.allLevels.push_back(lvl);
        } else {
            return eLvlInfo.error();
        }
    }
    if (!dstScene.isValid()) {
        return Error("Scene is not valid");
    }

    return NULLOPT;
}

// convert top-left coordinate to bottom-middle
Transform2D getTransformFromMapPosition(Vector2i position, Vector2i size, const ActiveLevel& level, bool isPoint) {
    // subtract (remember y=0 is top of map, so using +) half a tile of height to each point, since they describe the top of an object, but
    // Transform describes the bottom. Also Tiled is STUPID and uses different coordinate systems for tiles -- I turned on the setting for object
    // heights to match tiles

    Transform2D trans =
        Transform2D::pixels(position.x + size.x * 0.5 - PIXELS_PER_TILE / 2, level.size.y - position.y - size.y / 2 + PIXELS_PER_TILE / 2);
    trans.position += level.worldOffset;
    return trans;
}

// converts a relative sprite path to a valid GLResourceManager key
// example: "../sprite/actor/player-run1.png" -> "actor/player-run1"
// commented because i also commented out image-layer parsing
// std::string getSpriteKeyFromPath(const std::string& spritePath) {
//     const char* spriteDir = "sprite/";
//     constexpr s32 substrLen = 7;
//     const auto ix = spritePath.find(spriteDir);
//     if (ix == std::string::npos) {
//         return "";
//     }
//     const auto extensionIx = spritePath.find(".", ix + substrLen);
//     return spritePath.substr(ix + substrLen, extensionIx - ix - substrLen);
// }

// MAP LOADING STUFF

const nlohmann::json& getWorldFile(std::string_view mapFile) {
    const auto fullPath = whal_format("{}/{}", MAP_DIR, mapFile);
    return S_MAP_MANAGER.readData(fullPath.c_str());
}

const nlohmann::json& getMapFile(std::string_view mapFile) {
    // const auto fullPath = whal_format("{}/exports/{}", MAP_DIR, mapFile);
    const auto fullPath = whal_format("{}/{}", MAP_DIR, mapFile);
    return S_MAP_MANAGER.readData(fullPath.c_str());
}

// TEMPLATE STUFF

const nlohmann::json& getTemplate(std::string_view templateFile) {
    const auto fullPath = whal_format("{}/{}", MAP_DIR, templateFile);
    return S_TEMPLATE_MANAGER.readData(fullPath.c_str())["object"];
}

std::string getTypeFromTemplate(const std::string& templateFile) {
    const auto& prefabData = getTemplate(templateFile);
    std::string objType = "";
    tryReadString(prefabData, "type", &objType);
    return objType;
}

// reads size from object data, taking templates into account
Vector2i getObjectSize(const nlohmann::json& objectData) {
    Vector2i size;
    if (objectData.contains("template")) {
        auto templateFile = readString(objectData, "template");
        const auto& prefab = getTemplate(templateFile);
        tryReadVector2i(prefab, "width", "height", &size);
    }

    tryReadVector2i(objectData, "width", "height", &size);
    return size;
}

}  // namespace whal
