#include "Tiled.h"
#include <memory>

#include "Components/Collider.h"
#include "ECS.h"
#include "Systems/Graphics/TileRenderSystem.h"
#include "json.hpp"

#include "Settings.h"

#include "Components/Light.h"  // for level ambient lighting
#include "Components/Map.h"
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
static void loadObjectLayer(const nlohmann::json& layer, ecs::Entity parent, ActiveLevel* levelOpt = nullptr);
// static std::string getSpriteKeyFromPath(const std::string& spritePath);
static const std::shared_ptr<nlohmann::json> getTemplate(std::string_view templateFile);
static const std::shared_ptr<nlohmann::json> getMapFile(std::string_view mapFile);
static const std::shared_ptr<nlohmann::json> getWorldFile(std::string_view mapFile);
static std::string getTypeFromTemplate(const std::string& templateFile);
static Depth getLayerDepth(const nlohmann::json& layer, Depth defaultDepth);
static Depth loadTileLayerInfo(const nlohmann::json& data, ecs::Entity entity, TileMapLayer& layer);

void clearMapCache() {
    S_MAP_MANAGER.clearCache();
    S_TEMPLATE_MANAGER.clearCache();
}

static void addComponents(ecs::Entity entity, EntityMapData entityData, const nlohmann::json& object, const nlohmann::json& allObjects,
                          const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, ecs::Entity parent) {
    if (!object.contains("properties")) {
        return;
    }

    LoadContext ctx = {
        .values = nullptr,
        .allObjects = allObjects,
        .idToIndex = idToIndex,
        .entityData = entityData,
        .self = entity,
        .parent = parent,
        .isTiledData = true,
    };

    for (const auto& property : object["properties"]) {
        std::string componentName;
        if (!tryRead(property, "propertytype", &componentName)) {
            continue;
        }
        const std::optional<ComponentFactory::SerializeFuncs> serializerOpt = ComponentFactory::Get(componentName.c_str());
        if (!serializerOpt) {
            if (componentName == "InheritTemplate") {
                auto newTemplateFile = readString(property["value"], "TemplateFileName");
                std::string path = whal_format("templates/{}.tj", newTemplateFile);
                const auto newPrefab = getTemplate(path);
                addComponents(entity, entityData, *newPrefab, allObjects, idToIndex, parent);
                EntityBuilder builderFunc = nullptr;
                Prefab.entity.getEntry(newTemplateFile.c_str(), &builderFunc);
                if (builderFunc != nullptr) {
                    builderFunc(entity, *newPrefab, parent);
                }
            } else if (componentName == "SpriteMaskBlending") {
                // skip, handled in tileset logic
            } else {
                print("Skipping component", componentName, "because ComponentFactory has nothing with that name");
            }
            continue;
        }

        ctx.values = &property["value"];
        serializerOpt->load(entity, ctx);
    }
}

static void createTileMapLayerEntities(ecs::Entity layerEntity, ActiveLevel& level) {
    TileMapLayer& layer = layerEntity.get<TileMapLayer>();

    // dummy objects that tiles don't need because they are standalone entities
    const nlohmann::json emptyJson;
    const std::unordered_map<s32, std::pair<s32, ecs::Entity>> emptyIdToIndex;

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
            const s32 propsIx = tset.localIDToPropsIndex[localId];

            if (propsIx == -1) {
                // this tile doesn't have any extra properties
                continue;
            }

            const auto mapFile = getMapFile(tset.fileName);
            const auto& tiledata = (*mapFile)["tiles"][propsIx];
            // Tile has extra properties (e.g. a collider)
            // so we'll create a child entity to encapsulate this behavior
            ecs::Entity e = layerEntity.createChild(false);
            e.add<TileTag>();

            // level transform inherited from parent
            // get the tile's rotation
            std::pair<f32, Facing> orientation = getOrientation(tile);
            // if the layer has a z offset I'll remove the floatHeight and restore the old Y coordinate, because colliders ignore floatHeight (and for
            // non-visual components it really shouldn't matter)
            // e.get<Transform>().translate(Vector2f(x, -y + layer.zOffset) * FPIXELS_PER_TILE, e);
            e.set<Transform>(TransformBuilder(e.get<Transform>())
                                 .translate(Vector2f(x, -y + layer.zOffset) * FPIXELS_PER_TILE)
                                 .rotation(orientation.first)
                                 .facing(orientation.second)
                                 .build());
            const Vector2i mapPosition = Vector2i(x, y) * PIXELS_PER_TILE;
            const EntityMapData mapData = {
                .position = mapPosition,
                .size = {PIXELS_PER_TILE, PIXELS_PER_TILE},
                .id = 0,
                .isPoint = false,
                .isParsingTemplate = false,
            };

#ifndef NDEBUG
            e.add(Name{.name = whal_format("Tile ({}, {})", x, y)});
#endif

            // this call is safe even if the tile doesn't have any top-level components
            addComponents(e, mapData, tiledata, emptyJson, emptyIdToIndex, level.self);

            if (tiledata.contains("objectgroup")) {
                // tiles can have child objects (created with the collision editor)
                // for each of these, we'll create a child entity and add its components
                loadObjectLayer(tiledata["objectgroup"], e, nullptr);
            }

            // .has<> won't work for anything created in the collision editor
            // .getInChildren<> can be used instead
            // HACK created in colliion editor -> only blocks nav grid
            //      top level tile property -> affects occlusion mask and nav grid
            // I definitely want something more robust in the future
            if (e.has<Collider>()) {
                layer.occlusionMask[ix] = true;
                // e.get<Collider>().addLayer(CollisionLayer::BlocksVision); // not using this anymore
                level.navGrid[x][y] = false;
            } else if (e.getInChildren<Collider>(true)) {
                level.navGrid[x][y] = false;
            }
        }
    }
}

void TileMap::load(const char* path, ActiveLevel& level) {
    const auto data = getMapFile(path);

    std::shared_ptr<TileMap> map = std::make_shared<TileMap>();
    map->widthTiles = readInt(*data, "width");
    map->heightTiles = readInt(*data, "height");
    map->tileSize = readInt(*data, "tilewidth");

    for (const auto& tileset : (*data)["tilesets"]) {
        s32 firstgid = readInt(tileset, "firstgid");
        std::string fileName = readString(tileset, "source");

        TileSet tset = loadTileset(fileName, firstgid);
        map->tilesets.push_back(tset);
    }

    for (const auto& layer : (*data)["layers"]) {
        bool isVisible = readBool(layer, "visible");
        if (!isVisible) {
            continue;
        }
        std::string type = readString(layer, "type");

        if (type == "tilelayer") {
            // create an entity with a TileMapLayer component
            ecs::Entity layerEntity = level.self.createChild(false);
            auto _ = ecs::DeferActivate(layerEntity);
            layerEntity.add(Name(readString(layer, "name")));
            const Vector2i sizeTiles = {readInt(layer, "width"), readInt(layer, "height")};
            layerEntity.add(TileMapLayer{
                .sizeTiles = sizeTiles,
                .ids = layer["data"].get<std::vector<s32>>(),
                .tilemap = map,
                .occlusionMask = std::vector<bool>(sizeTiles.x * sizeTiles.y, false),
            });

            // this loads chunk size and other metadata:
            Transform& trans = layerEntity.get<Transform>();
            TileMapLayer& layerComponent = layerEntity.get<TileMapLayer>();
            trans.depth = loadTileLayerInfo(layer, layerEntity, layerComponent);
            if (layerComponent.zOffset != 0) {
                // if there's a Z offset (e.g. this layer has some height in the 3rd dimension), then move its actual position down and make it
                // "Float" so it's still rendered in the correct spot
                f32 positionOffset = static_cast<f32>(layerComponent.zOffset) * FPIXELS_PER_TILE;
                trans.translate(Vector2f(0, -positionOffset), layerEntity);
                trans.setFloatHeight(positionOffset / FLOAT_HEIGHT_MULT, layerEntity);
            }
            createTileMapLayerEntities(layerEntity, level);

            // proof of concept for a fun little stage transition:
            // this would look even cooler if the effect went right to left but that would be extra work
            // Schedule.tween(layerEntity, 0, 1, &Transform::rotation).from(-360).setTransition(Ease::InOutQuad);
            // Schedule.tween(layerEntity, Vector2f::ONE, 1, &Transform::scale).from(Vector2f::ZERO);

        } else if (type == "objectgroup") {
            loadObjectLayer(layer, level.self, &level);
        } else if (type == "imagelayer") {
            // parseImageLayer(layer, level);
            print("parseImageLayer disabled!");
        } else {
            print("unrecognized layer: ", type, "\nSkipping for now");
        }
    }

    // add ambient lighting for the level
    auto lightEntity = level.self.createChild();
    if (lightEntity.isValid()) {
        lightEntity.set(TransformBuilder(lightEntity.get<Transform>())
                            .translate(getMapTranslation(Vector2i::ZERO, level.size.as<s32>()))
                            .depth(Depth::Foreground2)
                            .build());

        BoxLight boxLight = {
            .radius = 3 * PIXELS_PER_TILE, .offset = Vector2i::ZERO, .color = level.meta.ambientLight, .halfLen = (level.size * 0.5).as<s32>()};
        lightEntity.add(boxLight);
        lightEntity.add(Name{.name = "BoxLight"});
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

Depth loadTileLayerInfo(const nlohmann::json& data, ecs::Entity entity, TileMapLayer& layer) {
    Depth layerDepth = Depth::Level;
    if (!data.contains("properties")) {
        print(entity.get<Name>(), "layer is missing TileMapInfo property (has no properties field)");
        return layerDepth;
    }

    for (const auto& property : data["properties"]) {
        std::string propertytype = readString(property, "propertytype");
        if (propertytype == "whal::TileMapLayer") {
            const auto& value = property["value"];
            tryRead(value, "Depth", &layerDepth);
            tryRead(value, "chunkSize", &layer.chunkSize);
            tryRead(value, "isYSorted", &layer.isYSorted);
            tryRead(value, "zOffset", &layer.zOffset);

            std::string overlayPath;
            if (tryRead(value, "overlayPath", &overlayPath)) {
                if (isExist(overlayPath.c_str())) {
                    auto errOpt = TextureManager::instance().loadAndRegister(overlayPath, overlayPath);
                    if (!errOpt) {
#ifdef __EMSCRIPTEN__
                        // GLES 2.0 only allows texture wrapping on power-of-two textures.
                        // Until I write some code to emulate that behavior, I will disable this feature on web builds for NPOT textures.
                        rl::Texture tex = TextureManager::getTexture(overlayPath);
                        if (math::isPowerOfTwo(tex.width) && math::isPowerOfTwo(tex.height)) {
                            layer.overlayTex = std::move(overlayPath);
                        }
#else
                        layer.overlayTex = std::move(overlayPath);
#endif
                    }
                }
            }

            return layerDepth;
        }
    }

    print(entity.get<Name>(), "layer is missing TileMapInfo property");
    return layerDepth;
}

// parses Tiled object info to create entities as children of the given parent.
// If levelOpt is not null*, then metadata will be parsed for the level as well.
// *should not be null when parsing a true object layer. Can be null when parsing a nested objectgroup (like tile collision data).
void loadObjectLayer(const nlohmann::json& layer, ecs::Entity parent, ActiveLevel* levelOpt) {
    using json = nlohmann::json;

    const Depth layerDepth = getLayerDepth(layer, Depth::Level);

    bool failedToAllocateEntities = false;
    const json& objects = layer["objects"];
    std::unordered_map<s32, std::pair<s32, ecs::Entity>> idToIndex;
    for (size_t ix = 0; ix < objects.size(); ix++) {
        s32 id = readInt(objects[ix], "id");

        ecs::Entity entity = parent.createChild(false);
        if (!entity.isValid()) {
            failedToAllocateEntities = true;
            break;
        }
        idToIndex.insert({id, {ix, entity}});
    }

    if (failedToAllocateEntities) {
        print("Failed to allocate entities for level ");

        // kill entities that we already made:
        for (auto [id, pair] : idToIndex) {
            pair.second.kill();
        }
        return;
    }

    for (const auto& object : objects) {
        EntityMapData entityMapData;
        entityMapData.id = readInt(object, "id");
        ecs::Entity entity = idToIndex.at(entityMapData.id).second;

        bool isVisible = true;
        if (tryRead(object, "visible", &isVisible) && !isVisible) {
            entity.kill();
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
            entity.kill();
            continue;
        }
        if (levelOpt != nullptr && objType != "Entity") {
            // check for metadata

            if (objType == "Map_CameraPoint") {
                Vector2i cameraPoint = readVector2i(object, "x", "y");
                levelOpt->cameraFocalPoint = (parent.get<Transform>().position + getMapTranslation(cameraPoint, Vector2i::ZERO)).as<s32>();
            }

            // not an entity, so kill it
            entity.kill();
            continue;
        }

        // check for prefab:
        std::shared_ptr<nlohmann::json> pPrefab = nullptr;
        if (object.contains("template")) {
            auto templateFile = readString(object, "template");
            pPrefab = getTemplate(templateFile);
        }

        // now get transform
        // check position/size in prefab first, then object
        bool hasPosition = false;
        entityMapData.isPoint = true;
        if (pPrefab) {
            hasPosition = tryRead(*pPrefab, "x", "y", &entityMapData.position);
        }
        hasPosition = tryRead(object, "x", "y", &entityMapData.position) || hasPosition;
        if (!hasPosition) {
            print("Entity with ID", entityMapData.id, "has no coordinates");
            entity.kill();
            continue;
        }
        if (pPrefab) {
            if (tryRead(*pPrefab, "width", "height", &entityMapData.size))
                entityMapData.isPoint = false;
        }
        if (tryRead(object, "width", "height", &entityMapData.size)) {
            entityMapData.isPoint = false;
        }

        f32 rotation = 0.0f;
        tryRead(object, "rotation", &rotation);

        // add transform
        entity.set(TransformBuilder(entity)
                       .translate(getMapTranslation(entityMapData.position, entityMapData.size))
                       .rotate(rotation)
                       .depth(layerDepth)
                       .build());

        // add name
        std::string name = "";
        if (pPrefab) {
            tryRead(*pPrefab, "name", &name);
        }
        tryRead(object, "name", &name);
        if (name.size() > 0) {
            entity.add(Name(name.c_str()));
            print("created entity: ", name);
        }

        if (pPrefab) {
            // add template components
            entityMapData.isParsingTemplate = true;
            addComponents(entity, entityMapData, *pPrefab, objects, idToIndex, parent);
            entityMapData.isParsingTemplate = false;

            // now run prefab factory function to do complicated stuff to components, like adding callbacks
            const auto prefabName = readString(*pPrefab, "name");
            EntityBuilder builderFunc = nullptr;
            Prefab.entity.getEntry(prefabName.c_str(), &builderFunc);
            if (builderFunc != nullptr) {
                builderFunc(entity, *pPrefab, parent);
            }
        }

        // add object components with factory
        addComponents(entity, entityMapData, object, objects, idToIndex, parent);
    }
}

TileSet loadTileset(const std::string& basename, s32 firstgid) {
    const auto data = getMapFile(basename);

    auto sourceFilePath = readString(*data, "image");

    const s32 firstIx = std::max<s32>(sourceFilePath.find_last_of('/'), sourceFilePath.find_last_of('\\')) + 1;
    const s32 lastIx = sourceFilePath.find(".", firstIx);
    const std::string sourceFileBasenameNoExt = sourceFilePath.substr(firstIx, lastIx - firstIx);

    const s32 tileWidth = readInt(*data, "tilewidth");
    const s32 tileHeight = readInt(*data, "tileheight");
    const s32 width = readInt(*data, "imagewidth");
    const s32 height = readInt(*data, "imageheight");

    const s32 widthTiles = width / tileWidth;
    const s32 heightTiles = height / tileHeight;
    const s32 tilecount = readInt(*data, "tilecount");

    std::vector<s32> idToIx(tilecount, -1);
    if (data->contains("tiles")) {
        s32 ix = 0;
        for (const auto& tiledata : (*data)["tiles"]) {
            s32 id = readInt(tiledata, "id");
            idToIx[id] = ix++;
        }
    }

    std::string maskPath = "";
    bool isAdditiveSpriteMask = false;
    if (data->contains("properties")) {
        for (const auto& prop : (*data)["properties"]) {
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
        .margin = (*data)["margin"],
        .spacing = (*data)["spacing"],
        .fileName = basename,
        .spriteFileName = std::move(sourceFileBasenameNoExt),
        .spriteMaskFileName = std::move(maskPath),
        .localIDToPropsIndex = std::move(idToIx),
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
    return TiledDataType::Int;
}

// parses all the data types in a project
void parseMapProject(const char* mapfile) {
    const auto data = getWorldFile(mapfile);
    for (const auto& propType : (*data)["propertyTypes"]) {
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
                        // TODO if it's an enum I also need to store the propertytype
                        // if the enum is stored as an int, memberType will be an int, but I'll also need propertytype
                        // and if the enum is stored as a string, memberType will be a string
                        // so I need to double check if memberPropType is in ComponentFactory::propertyTypes
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
static Expected<Level::MetaData> parseLevelInfo(const char* lvlFileName) {
    const auto data = getMapFile(lvlFileName);
    for (auto& property : (*data)["properties"]) {
        std::string propType = readString(property, "propertytype");
        if (propType == "Map_MapInfo") {
            auto mapInfo = property["value"];
            Level::MetaData lvlInfo;
            tryRead(mapInfo, "AmbientLight", &lvlInfo.ambientLight);
            tryRead(mapInfo, "isWorldEntryPoint", &lvlInfo.isWorldEntryPoint);
            return lvlInfo;
        }
    }

    return Error(whal_format("Map_MapInfo property not found in level: {}", lvlFileName));
}

Corrade::Containers::Optional<Error> parseWorld(const char* mapfile, Scene& dstScene) {
    const auto data = getWorldFile(mapfile);
    dstScene.self.add<Name>({mapfile});

#ifndef NDEBUG
    std::string type = readString(*data, "type");
    if (type != "world") {
        return Error("Not a world file");
    }
#endif

    dstScene.name = mapfile;
    for (auto& map : (*data)["maps"]) {
        std::string filename = map["fileName"];
        s32 x = readInt(map, "x");
        s32 y = readInt(map, "y");
        s32 width = readInt(map, "width");
        s32 height = readInt(map, "height");
        Expected<Level::MetaData> eLvlInfo = parseLevelInfo(filename.c_str());
        if (eLvlInfo.isExpected()) {
            Level lvl = {filename, Vector2f(x, -y), Vector2f(width, height), eLvlInfo.value()};
            if (lvl.meta.isWorldEntryPoint) {
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

Transform getMapTransform(Vector2i mapPosition, Vector2i entitySize, ecs::Entity parent) {
    return TransformBuilder(parent).translate(getMapTranslation(mapPosition, entitySize)).build();
}

// converts relative map position (where origin is top-left) to relative world position (origin is the middle)
Vector2f getMapTranslation(Vector2i mapPosition, Vector2i entitySize) {
    return Vector2f(mapPosition.x + entitySize.x * 0.5 - PIXELS_PER_TILE / 2, -mapPosition.y - entitySize.y / 2 + PIXELS_PER_TILE / 2);
}

// MAP LOADING STUFF

const std::shared_ptr<nlohmann::json> getWorldFile(std::string_view mapFile) {
    const auto fullPath = whal_format("{}/{}", MAP_DIR, mapFile);
    return S_MAP_MANAGER.readData(fullPath.c_str());
}

const std::shared_ptr<nlohmann::json> getMapFile(std::string_view mapFile) {
    // const auto fullPath = whal_format("{}/exports/{}", MAP_DIR, mapFile);
    const auto fullPath = whal_format("{}/{}", MAP_DIR, mapFile);
    return S_MAP_MANAGER.readData(fullPath.c_str());
}

// TEMPLATE STUFF

const std::shared_ptr<nlohmann::json> getTemplate(std::string_view templateFile) {
    const auto fullPath = whal_format("{}/{}", MAP_DIR, templateFile);
    return std::make_shared<nlohmann::json>((*S_TEMPLATE_MANAGER.readData(fullPath.c_str()))["object"]);
}

std::string getTypeFromTemplate(const std::string& templateFile) {
    const auto prefabData = getTemplate(templateFile);
    std::string objType = "";
    tryRead(*prefabData, "type", &objType);
    return objType;
}

// reads size from object data, taking templates into account
Vector2i getObjectSize(const nlohmann::json& objectData) {
    Vector2i size;
    if (objectData.contains("template")) {
        auto templateFile = readString(objectData, "template");
        const auto prefab = getTemplate(templateFile);
        tryRead(*prefab, "width", "height", &size);
    }

    tryRead(objectData, "width", "height", &size);
    return size;
}

}  // namespace whal
