#include "Tiled.h"
#include <memory>

#include "ECS.h"
#include "json.hpp"

#include "Settings.h"

#include "Components/Light.h"  // for level ambient lighting
#include "Components/MapLayer.h"
#include "Components/Name.h"
#include "Components/Relationships.h"
#include "Components/Transform.h"

#include "Gfx/Depth.h"
#include "Gfx/Frame.h"
#include "Gfx/Texture.h"

#include "Map/ComponentFactory.h"
#include "Map/EntityFactory.h"
#include "Map/Level.h"
#include "TiledParse.h"

#include "Sys/System.h"

#include "Util/DebugUtil.h"
#include "Util/Print.h"
#include "Util/ResourceManager.h"

#define NULLOPT Corrade::Containers::NullOpt;

namespace whal {

static const char* MAP_DIR = "data/map";

static ResourceManager<nlohmann::json, 50> S_TEMPLATE_MANAGER;
static ResourceManager<nlohmann::json, 250> S_MAP_MANAGER;
std::unordered_map<std::string, PropertyType> ComponentFactory::propertyTypes = {};
std::unordered_map<std::string, std::pair<TiledDataType, std::string>> ComponentFactory::memberTypes = {};

static TileSet loadTileset(const std::string& basename, s32 firstgid);
static void loadObjectLayer(const nlohmann::json& layer, ActiveLevel& level);
// static std::string getSpriteKeyFromPath(const std::string& spritePath);
static const nlohmann::json& getTemplate(std::string_view templateFile);
static const nlohmann::json& getMapFile(std::string_view mapFile);
static const nlohmann::json& getWorldFile(std::string_view mapFile);
static std::string getTypeFromTemplate(const std::string& templateFile);
static Depth getLayerDepth(const nlohmann::json& layer, Depth defaultDepth);
static Depth loadTileLayerInfo(const nlohmann::json& data, TileMapLayer& layer);

void clearMapCache() {
    S_MAP_MANAGER.clearCache();
    S_TEMPLATE_MANAGER.clearCache();
}

static void addComponents(ecs::Entity entity, EntityMapData entityData, const nlohmann::json& object, const nlohmann::json& allObjects,
                          const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, const ActiveLevel& level) {
    if (!object.contains("properties")) {
        return;
    }

    for (auto& property : object["properties"]) {
        std::string componentName;
        if (!tryRead(property, "propertytype", &componentName)) {
            continue;
        }
        auto serializerOpt = ComponentFactory::Get(componentName.c_str());
        if (!serializerOpt) {
            if (componentName == "InheritTemplate") {
                auto newTemplateFile = readString(property["value"], "TemplateFileName");
                std::string path = whal_format("templates/{}.tj", newTemplateFile);
                const auto& newPrefab = getTemplate(path);
                addComponents(entity, entityData, newPrefab, allObjects, idToIndex, level);
                EntityBuilder builderFunc = nullptr;
                Prefab.entity.getEntry(newTemplateFile.c_str(), &builderFunc);
                if (builderFunc != nullptr) {
                    builderFunc(entity, newPrefab, level);
                }
            } else if (componentName == "SpriteMaskBlending") {
                // skip, handled in tileset logic
            } else {
                print("Skipping component", componentName, "because ComponentFactory has nothing with that name");
            }
            continue;
        }

        const LoadContext ctx = {
            .values = property["value"],
            .allObjects = allObjects,
            .idToIndex = idToIndex,
            .entityData = entityData,
            .level = level,
            .isTiledData = true,
        };

        serializerOpt->load(entity, (void*)&ctx);  // >:)
    }
}

static void createTileMapLayerEntities(ecs::Entity layerEntity, ActiveLevel& level) {
    const Transform& layerTrans = layerEntity.get<Transform>();
    const Vector2f origin = layerTrans.position;
    TileMapLayer& layer = layerEntity.get<TileMapLayer>();

    // dummy objects that tiles don't need because they are standalone entities
    const nlohmann::json emptyJson;
    const std::unordered_map<s32, std::pair<s32, ecs::Entity>> emptyIdToIndex;

    // TODO navgrid should be owned by layerEntity
    for (s32 x = 0; x < layer.tilemap->widthTiles; x++) {
        level.navGrid.push_back(std::vector<bool>(layer.tilemap->heightTiles, true));

        for (s32 y = 0; y < layer.tilemap->heightTiles; y++) {
            const s32 ix = layer.tilemap->widthTiles * y + x;
            u32 tileMask = layer.ids[ix];
            TileInfo tile = getTile(tileMask);

            if (tile.gid == 0) {
                continue;  // empty tile
            }

            const TileSet& tset = getTileSet(*layer.tilemap.get(), tile.gid);
            const s32 localId = tile.gid - tset.firstgid;
            const s32 propsIx = tset.tileIDToIndex[localId];
            if (propsIx == -1) {
                // this tile doesn't have any extra properties
                continue;
            }

            // Tile has extra properties (e.g. a collider)
            // so we'll create a child entity to encapsulate this behavior
            ecs::Entity e = layerEntity.createChild(false);
            e.get<Transform>().setPosition(Vector2f(x * PIXELS_PER_TILE, -y * PIXELS_PER_TILE) + origin, e);
            const Vector2i mapPosition = Vector2i(x * PIXELS_PER_TILE, y * PIXELS_PER_TILE);
            const EntityMapData mapData = {mapPosition, {PIXELS_PER_TILE, PIXELS_PER_TILE}, 0, false, true};

            addComponents(e, mapData, getMapFile(tset.fileName)["tiles"][propsIx], emptyJson, emptyIdToIndex, level);

            // For simplicity, tiles with collision also block light, vision, and pathing
            if (e.has<Collider>()) {
                layer.collisionMask[ix] = true;
                e.get<Collider>().setCollisionMask(CollisionLayer::BlocksVision);
                level.navGrid[x][y] = false;
            }
        }
    }
}

void TileMap::load(const char* path, ActiveLevel& level) {
    const auto& data = getMapFile(path);

    std::shared_ptr<TileMap> map = std::make_shared<TileMap>();
    map->widthTiles = readInt(data, "width");
    map->heightTiles = readInt(data, "height");
    map->tileSize = readInt(data, "tilewidth");

    for (const auto& tileset : data["tilesets"]) {
        s32 firstgid = readInt(tileset, "firstgid");
        std::string fileName = readString(tileset, "source");

        TileSet tset = loadTileset(fileName, firstgid);
        map->tilesets.push_back(tset);
    }

    const Vector2f origin(level.worldPosOrigin.x, level.worldPosOrigin.y - level.size.y);

    for (const auto& layer : data["layers"]) {
        bool isVisible = readBool(layer, "visible");
        if (!isVisible) {
            continue;
        }
        std::string type = readString(layer, "type");

        if (type == "tilelayer") {
            // create an entity with a TileMapLayer component
            ecs::Entity layerEntity = World.entity(false);
            auto _ = ecs::DeferActivate(layerEntity);
            const Vector2i sizeTiles = {readInt(layer, "width"), readInt(layer, "height")};
            layerEntity.add(TileMapLayer{
                .sizeTiles = sizeTiles,
                .ids = layer["data"].get<std::vector<s32>>(),
                .tilemap = map,
                .collisionMask = std::vector<bool>(sizeTiles.x * sizeTiles.y, false),
            });
            layerEntity.add(Name(readString(layer, "name")));
            // TODO transform should be the center of the layer, not the top left (?) corner
            Transform trans = Transform(Transform::tiles(0, map->heightTiles).position + origin);

            // this loads chunk size and other metadata:
            trans.depth = loadTileLayerInfo(layer, layerEntity.get<TileMapLayer>());

            layerEntity.set(trans);
            level.childEntities.insert(layerEntity);
            createTileMapLayerEntities(layerEntity, level);

            // proof of concept for a fun little stage transition:
            // this would look even cooler if the effect went right to left but that would be extra work
            // Schedule.tween(layerEntity, 0, 1, &Transform::rotation).from(-360).setTransition(Ease::InOutQuad);
            // Schedule.tween(layerEntity, Vector2f::ONE, 1, &Transform::scale).from(Vector2f::ZERO);

        } else if (type == "objectgroup") {
            loadObjectLayer(layer, level);
        } else if (type == "imagelayer") {
            // parseImageLayer(layer, level);
            print("parseImageLayer disabled!");
        } else {
            print("unrecognized layer: ", type, "\nSkipping for now");
        }
    }

    // add ambient lighting for the level
    auto lightEntity = World.entity();
    if (lightEntity.isValid()) {
        // idk why but i need 1 tile of extra height
        auto trans = Transform::world(level.worldOffset + (level.size * 0.5 + Vector2f(-FPIXELS_PER_TILE / 2, FPIXELS_PER_TILE)).as<s32>());
        trans.depth = Depth::Foreground2;
        lightEntity.set(trans);

        BoxLight boxLight = {
            .radius = 3 * PIXELS_PER_TILE, .heightOffset = 0, .color = level.lvlInfo.ambientLight, .halfLen = (level.size * 0.5).as<s32>()};
        lightEntity.add(boxLight);

        level.childEntities.insert(lightEntity);
    } else {
        print("Couldn't allocate entity for level lighting");
    }
}

Depth getLayerDepth(const nlohmann::json& layer, Depth defaultDepth) {
    Depth layerDepth = defaultDepth;
    if (layer.contains("properties")) {
        for (auto& property : layer["properties"]) {
            std::string propertytype = readString(property, "propertytype");
            if (propertytype == "Depth") {
                layerDepth = readDepth(property["value"]);
                break;
            }
        }
    }
    return layerDepth;
}

Depth loadTileLayerInfo(const nlohmann::json& data, TileMapLayer& layer) {
    Depth layerDepth = Depth::Level;
    if (!data.contains("properties")) {
        print("layer is missing TileMapInfo property");
        return layerDepth;
    }

    for (const auto& property : data["properties"]) {
        std::string propertytype = readString(property, "propertytype");
        if (propertytype == "whal::TileMapLayer") {
            const auto& value = property["value"];
            tryRead(value, "Depth", &layerDepth);
            tryRead(value, "chunkSize", &layer.chunkSize);
            tryRead(value, "isYSorted", &layer.isYSorted);

            return layerDepth;
        }
    }

    print("layer is missing TileMapInfo property");
    return layerDepth;
}

// this will create entities and immediately add them to the level
void loadObjectLayer(const nlohmann::json& layer, ActiveLevel& level) {
    using json = nlohmann::json;

    const Depth layerDepth = getLayerDepth(layer, Depth::Level);

    bool failedToAllocateEntities = false;
    const json& objects = layer["objects"];
    std::unordered_map<s32, std::pair<s32, ecs::Entity>> idToIndex;
    for (size_t ix = 0; ix < objects.size(); ix++) {
        s32 id = readInt(objects[ix], "id");

        auto entity = World.entity(false);
        if (!entity.isValid()) {
            failedToAllocateEntities = true;
            break;
        }
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
        if (tryRead(object, "visible", &isVisible) && !isVisible) {
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
                level.cameraFocalPoint = getTransformFromMapPosition(cameraPoint, {0, 0}, level, true).positionPx;
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
            hasPosition = tryRead(*pPrefab, "x", "y", &entityData.position);
        }
        hasPosition = tryRead(object, "x", "y", &entityData.position) || hasPosition;
        if (!hasPosition) {
            print("Entity with ID", entityData.id, "has no coordinates");
            entity.kill();
            continue;
        }
        if (pPrefab) {
            if (tryRead(*pPrefab, "width", "height", &entityData.size))
                entityData.isPoint = false;
        }
        if (tryRead(object, "width", "height", &entityData.size))
            entityData.isPoint = false;

        // add transform
        Transform trans = getTransformFromMapPosition(entityData.position, entityData.size, level, entityData.isPoint);
        trans.depth = layerDepth;
        entity.set(trans);

        // add name
        std::string name = "";
        if (tryRead(object, "name", &name)) {
            entity.add(Name(name.c_str()));
            print("created entity: ", name);
        }

        if (pPrefab) {
            // add template components
            entityData.isParsingTemplate = true;
            addComponents(entity, entityData, *pPrefab, objects, idToIndex, level);
            entityData.isParsingTemplate = false;

            // now run prefab factory function to do complicated stuff to components, like adding callbacks
            const auto prefabName = readString(*pPrefab, "name");
            EntityBuilder builderFunc = nullptr;
            Prefab.entity.getEntry(prefabName.c_str(), &builderFunc);
            if (builderFunc != nullptr) {
                // print("got template entry for", name);
                builderFunc(entity, *pPrefab, level);
                // } else {
                //     print("did NOT got template entry for", name);
                //     print("prefab name is ", prefabName.c_str());
            }
            // } else {
            //     print(name, "does not have prefab");
        }

        // add object components with factory
        addComponents(entity, entityData, object, objects, idToIndex, level);
    }
}

TileSet loadTileset(const std::string& basename, s32 firstgid) {
    const auto& data = getMapFile(basename);

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

    std::string maskPath = "";
    bool isAdditiveSpriteMask = false;
    if (data.contains("properties")) {
        for (const auto& prop : data["properties"]) {
            if (prop["name"] == "SpriteMask") {
                maskPath = prop["value"];
            } else if (prop["name"] == "SpriteMaskBlending") {
                std::string blending = prop["value"];
                if (blending == "Add") {
                    isAdditiveSpriteMask = true;
                }
            }
        }
    }

    return TileSet{
        .firstgid = firstgid,
        .tilecount = tilecount,
        .tileWidth = tileWidth,
        .tileHeight = tileHeight,
        .widthTiles = widthTiles,
        .heightTiles = heightTiles,
        .margin = data["margin"],
        .spacing = data["spacing"],
        .fileName = basename,
        .spriteFileName = std::move(sourceFileBasenameNoExt),
        .spriteMaskFileName = std::move(maskPath),
        .tileIDToIndex = std::move(idToIx),
        .isAdditiveSpriteMask = isAdditiveSpriteMask,
    };
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

Expected<Sprite> getTileSprite(const TileMap& map, s32 blockId) {
    const TileSet& tset = getTileSet(map, blockId);
    std::string spritePath = whal_format("{}/{}", "map", tset.spriteFileName);
    const auto& texAtlas = TextureManager::getAtlas(TEXNAME_SPRITE);
    Corrade::Containers::Optional<rl::Rectangle> tsetFrameOpt = texAtlas.getFrame(spritePath.c_str());

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

    Sprite sprite = Sprite::fromFrame(newFrame);

    // check if there's a sprite mask for this tileset
    if (tset.spriteMaskFileName != "") {
        auto maskFrameOpt = texAtlas.getFrame(tset.spriteMaskFileName.c_str());
        if (!maskFrameOpt) {
            print("Couldn't find", tset.spriteMaskFileName, "in texture atlas");
        } else {
            Frame fullMaskFrame = *maskFrameOpt;
            Frame maskFrame = {{fullMaskFrame.atlasPosition.x + colIx * tset.tileWidth, fullMaskFrame.atlasPosition.y + rowIx * tset.tileHeight},
                               {tset.tileWidth, tset.tileHeight}};
            sprite.setMask(maskFrame);
            if (tset.isAdditiveSpriteMask) {
                sprite.setFlag(Sprite::MaskBlendAdditive);
            }
        }
    }

    return sprite;
}

static TiledDataType getDtype(const std::string& name) {
    if (name == "int") {
        return TiledDataType::Int;
    } else if (name == "string") {
        return TiledDataType::String;
    } else if (name == "float") {
        return TiledDataType::Float;
    } else if (name == "bool") {
        return TiledDataType::Bool;
    } else if (name == "object") {
        return TiledDataType::Object;
    } else if (name == "enum") {
        return TiledDataType::Enum;
    } else if (name == "color") {
        return TiledDataType::Color;
    } else if (name == "class") {
        return TiledDataType::Class;
    }
    DBG_ASSERT(false, whal_format("unrecognized property type: {}", name));
}

// parses all the data types in a project
void parseMapProject(const char* mapfile) {
    const auto data = getWorldFile(mapfile);
    for (const auto& propType : data["propertyTypes"]) {
        const std::string name = readString(propType, "name");
        const TiledDataType dtype = getDtype(propType["type"]);
        if (dtype == TiledDataType::Enum) {
            // for enums, check how the data is stored (packed int, basic int, or string)
            const std::string storage = readString(propType, "storageType");
            const bool isString = storage == "string";
            const bool isFlags = readBool(propType, "valuesAsFlags");

            ComponentFactory::propertyTypes.insert({std::move(name), PropertyType{
                                                                         .dtype = dtype,
                                                                         .enumInfo =
                                                                             {
                                                                                 .isString = isString,
                                                                                 .isFlags = isFlags,
                                                                             },
                                                                     }});
        } else if (dtype == TiledDataType::Class) {
            if (propType.contains("members")) {
                // parse types of class members
                for (const auto& member : propType["members"]) {
                    std::string memberName = readString(member, "name");
                    memberName = name + ":" + memberName;
                    const TiledDataType memberType = getDtype(member["type"]);
                    if (memberType == TiledDataType::Class) {
                        // this is a class, so we need to store its propertyType
                        std::string memberPropType = "";  // is possible that it's null
                        tryRead(member, "propertyType", &memberPropType);
                        ComponentFactory::memberTypes.insert({std::move(memberName), {memberType, std::move(memberPropType)}});
                    } else {
                        // propertyType doesn't matter, do empty string
                        ComponentFactory::memberTypes.insert({std::move(memberName), {memberType, ""}});
                    }
                }
            }
            ComponentFactory::propertyTypes.insert(
                {std::move(name), PropertyType{.dtype = dtype, .enumInfo = {.isString = false, .isFlags = false}}});

        } else {
            // for other types, just the dtype is enough information
            ComponentFactory::propertyTypes.insert(
                {std::move(name), PropertyType{.dtype = dtype, .enumInfo = {.isString = false, .isFlags = false}}});
        }
    }
}

// parses a level's parameters and returns its LevelInfo struct
static Expected<Level::LevelInfo> parseLevelInfo(const char* lvlFileName) {
    const auto& data = getMapFile(lvlFileName);
    for (auto& property : data["properties"]) {
        std::string propType = readString(property, "propertytype");
        if (propType == "Map_MapInfo") {
            auto mapInfo = property["value"];
            Level::LevelInfo lvlInfo;
            tryRead(mapInfo, "AmbientLight", &lvlInfo.ambientLight);
            tryRead(mapInfo, "isWorldEntryPoint", &lvlInfo.isWorldEntryPoint);
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
Transform getTransformFromMapPosition(Vector2i position, Vector2i size, const ActiveLevel& level, bool isPoint) {
    // subtract (remember y=0 is top of map, so using +) half a tile of height to each point, since they describe the top of an object, but
    // Transform describes the bottom. Also Tiled is STUPID and uses different coordinate systems for tiles -- I turned on the setting for object
    // heights to match tiles

    Transform trans = Transform::world(
        Vector2i(position.x + size.x * 0.5 - PIXELS_PER_TILE / 2, level.size.y - position.y - size.y / 2 + PIXELS_PER_TILE / 2) + level.worldOffset);
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
    tryRead(prefabData, "type", &objType);
    return objType;
}

// reads size from object data, taking templates into account
Vector2i getObjectSize(const nlohmann::json& objectData) {
    Vector2i size;
    if (objectData.contains("template")) {
        auto templateFile = readString(objectData, "template");
        const auto& prefab = getTemplate(templateFile);
        tryRead(prefab, "width", "height", &size);
    }

    tryRead(objectData, "width", "height", &size);
    return size;
}

}  // namespace whal
