#include "Tiled.h"

#include "Components/Collider.h"
#include "Components/Light.h"  // for level ambient lighting
#include "Components/Map.h"
#include "Components/Relationships.h"
#include "Components/Tags.h"
#include "Components/Transform.h"
#include "ECS.h"
#include "Gfx/Depth.h"
#include "Gfx/Frame.h"
#include "Gfx/Texture.h"
#include "Map/ComponentFactory.h"
#include "Map/EntityFactory.h"
#include "Map/Level.h"
#include "Settings.h"
#include "Sys/Prefab.h"
#include "Sys/System.h"
#include "Systems/Graphics/TileRenderSystem.h"
#include "TiledParse.h"
#include "Util/DebugUtil.h"
#include "Util/JsonDoc.h"
#include "Util/JsonUtil.h"
#include "Util/Print.h"
#include "Util/ResourceManager.h"

using Corrade::Containers::NullOpt;

namespace whal {

static const char* MAP_DIR = "data/map";

static ResourceManager<JsonDoc, 50> S_TEMPLATE_MANAGER;
static ResourceManager<JsonDoc, 250> S_MAP_MANAGER;
std::unordered_map<std::string, PropertyType> ComponentFactory::propertyTypes = {};
std::unordered_map<std::string, std::pair<TiledDataType, std::string>> ComponentFactory::memberTypes = {};

static TileSet loadTileset(const std::string& basename, s32 firstgid);
static void loadObjectLayer(JsonValue layer, ecs::Entity parent, ActiveLevel* levelOpt = nullptr);
// static std::string getSpriteKeyFromPath(const std::string& spritePath);
static const Arc<JsonValue> getTemplate(std::string_view templateFile);
static const Arc<JsonDoc> getMapFile(std::string_view mapFile);
static const Arc<JsonDoc> getWorldFile(std::string_view mapFile);
static std::string getTypeFromTemplate(const std::string& templateFile);
static Depth loadTileLayerInfo(const JsonValue data, ecs::Entity entity, TileMapLayer& layer);

void clearMapCache() {
    S_MAP_MANAGER.clearCache();
    S_TEMPLATE_MANAGER.clearCache();
}

static void addComponents(ecs::Entity entity, EntityMapData entityData, JsonValue object, JsonValue allObjects,
                          const std::unordered_map<s32, std::pair<s32, ecs::Entity>>& idToIndex, ecs::Entity parent) {
    if (!object.contains("properties")) {
        return;
    }

    LoadContext ctx = {
        .values = JsonValue(),
        .allObjects = allObjects,
        .idToIndex = idToIndex,
        .entityData = entityData,
        .self = entity,
        .parent = parent,
    };

    for (JsonValue property : object["properties"]) {
        std::string componentName;
        if (!tryRead(property, "propertytype", &componentName)) {
            continue;
        }
        const TiledDeserialize* serializerOpt = ComponentFactory::get(componentName.c_str());
        if (!serializerOpt) {
            if (componentName == "InheritTemplate") {
                std::string newTemplateFile = readString(property["value"], "TemplateFileName");
                std::string path = whal_format("templates/{}.tj", newTemplateFile);
                const Arc<JsonValue> newPrefab = getTemplate(path);
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

        ctx.values = property["value"];
        serializerOpt->load(entity, ctx);
    }
}

static void createTileMapLayerEntities(ecs::Entity layerEntity, ActiveLevel& level) {
    TileMapLayer& layer = layerEntity.get<TileMapLayer>();

    // dummy objects that tiles don't need because they are standalone entities
    JsonValue emptyJson;
    const std::unordered_map<s32, std::pair<s32, ecs::Entity>> emptyIdToIndex;

    for (s32 x = 0; x < level.sizeTiles.x; x++) {
        for (s32 y = 0; y < level.sizeTiles.y; y++) {
            const s32 ix = level.sizeTiles.x * y + x;
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

            const Arc<JsonDoc> mapFile = getMapFile(tset.fileName);
            const JsonValue tiledata = mapFile->getRoot()["tiles"][propsIx];
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
            e.setName(whal_format("Tile ({}, {})", x, y).c_str());
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
    const Arc<JsonDoc> data = getMapFile(path);

    Arc<TileMap> map = Arc<TileMap>::New();
    JsonValue root = data->getRoot();
    map->widthTiles = readInt(root, "width");
    map->heightTiles = readInt(root, "height");
    map->tileSize = readInt(root, "tilewidth");

    // initialize the navigation grid
    level.navGrid.clear();
    for (s32 x = 0; x < level.sizeTiles.x; x++) {
        level.navGrid.push_back(std::vector<u8>(level.sizeTiles.y, 1));
    }

    for (JsonValue tileset : root["tilesets"]) {
        s32 firstgid = readInt(tileset, "firstgid");
        std::string fileName = readString(tileset, "source");

        TileSet tset = loadTileset(fileName, firstgid);
        map->tilesets.push_back(tset);
    }

    for (JsonValue layer : root["layers"]) {
        bool isVisible = readBool(layer, "visible");
        if (!isVisible) {
            continue;
        }
        std::string type = readString(layer, "type");

        if (type == "tilelayer") {
            // create an entity with a TileMapLayer component
            ecs::Entity layerEntity = level.self.createChild(readString(layer, "name").c_str(), false);
            auto _ = ecs::DeferActivate(layerEntity);
            const Vector2i sizeTiles = {readInt(layer, "width"), readInt(layer, "height")};
            layerEntity.add(TileMapLayer{
                .sizeTiles = sizeTiles,
                // .ids = layer["data"].get<std::vector<s32>>(),
                .ids = jsonPropertyToVector<s32>(layer, "data"),
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
            ecs::Entity layerEntity = level.self.createChild(readString(layer, "name").c_str());
            layerEntity.add<TiledObjectLayer>();
            loadObjectLayer(layer, layerEntity, &level);

        } else if (type == "imagelayer") {
            // parseImageLayer(layer, level);
            print("parseImageLayer disabled!");

        } else {
            print("unrecognized layer: ", type, "\nSkipping for now");
        }
    }

    // add ambient lighting for the level
    ecs::Entity lightEntity = level.self.createChild("BoxLight");
    if (lightEntity.isValid()) {
        lightEntity.set(TransformBuilder(lightEntity.get<Transform>())
                            .translate(getMapTranslation(Vector2i::ZERO, level.size.as<s32>()))
                            .depth(Depth::Foreground2)
                            .build());

        BoxLight boxLight = {
            .radius = 3 * PIXELS_PER_TILE, .offset = Vector2i::ZERO, .color = level.ambientLight, .halfLen = (level.size * 0.5).as<s32>()};
        lightEntity.add(boxLight);
    } else {
        print("Couldn't allocate entity for level lighting");
    }
}

Depth loadTileLayerInfo(JsonValue data, ecs::Entity entity, TileMapLayer& layer) {
    Depth layerDepth = Depth::Level;
    if (!data.contains("properties")) {
        print(entity.name(), "layer is missing TileMapInfo property (has no properties field)");
        return layerDepth;
    }

    for (JsonValue property : data["properties"]) {
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
                    } else {
                        print("Error loading overlay path: ", *errOpt);
                    }
                }
            }

            return layerDepth;
        }
    }

    print(entity.name(), "layer is missing TileMapInfo property");
    return layerDepth;
}

// parses Tiled object info to create entities as children of the given parent.
// If levelOpt is not null*, then metadata will be parsed for the level as well.
// *should not be null when parsing a true object layer. Can be null when parsing a nested objectgroup (like tile collision data).
void loadObjectLayer(JsonValue layer, ecs::Entity parent, ActiveLevel* levelOpt) {
    Transform& parentTrans = parent.get<Transform>();

    // only need to parse depth for real layers (not tile collision data)
    Depth layerDepth = Depth::Level;
    if (levelOpt != nullptr) {
        // Not everything in TileMapLayer is relevant to object layers
        TileMapLayer tmpLayer;
        layerDepth = loadTileLayerInfo(layer, parent, tmpLayer);
        if (tmpLayer.zOffset != 0) {
            // if there's a Z offset (e.g. this layer has some height in the 3rd dimension), then move its actual position down and make it
            // "Float" so it's still rendered in the correct spot
            f32 positionOffset = static_cast<f32>(tmpLayer.zOffset) * FPIXELS_PER_TILE;
            parentTrans.translate(Vector2f(0, -positionOffset), parent);
            parentTrans.setFloatHeight(positionOffset / FLOAT_HEIGHT_MULT, parent);
        }
    }

    bool failedToAllocateEntities = false;
    JsonValue objects = layer["objects"];
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
                levelOpt->cameraFocalPoint = (parentTrans.position + getMapTranslation(cameraPoint, Vector2i::ZERO)).as<s32>();
            }

            // not an entity, so kill it
            entity.kill();
            continue;
        }

        // check for prefab:
        Arc<JsonValue> pPrefab = nullptr;
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
        Transform trans = TransformBuilder(entity)
                              .translate(getMapTranslation(entityMapData.position, entityMapData.size))
                              .rotate(rotation)
                              .depth(layerDepth)
                              .build();
        entity.set(trans);

        // add initial position
        entity.add<TileMapObject>({
            .initialTransform = trans,
            .mapFile = levelOpt ? levelOpt->filepath : "",
        });

        // add name
        std::string name = "";
        if (pPrefab) {
            tryRead(*pPrefab, "name", &name);
        }
        tryRead(object, "name", &name);
        if (name.size() > 0) {
            entity.setName(name.c_str());
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
    const Arc<JsonDoc> data = getMapFile(basename);
    JsonValue root = **data;

    auto sourceFilePath = readString(root, "image");

    const s32 firstIx = std::max<s32>(sourceFilePath.find_last_of('/'), sourceFilePath.find_last_of('\\')) + 1;
    const s32 lastIx = sourceFilePath.find(".", firstIx);
    const std::string sourceFileBasenameNoExt = sourceFilePath.substr(firstIx, lastIx - firstIx);

    const s32 tileWidth = readInt(root, "tilewidth");
    const s32 tileHeight = readInt(root, "tileheight");
    const s32 width = readInt(root, "imagewidth");
    const s32 height = readInt(root, "imageheight");

    const s32 widthTiles = width / tileWidth;
    const s32 heightTiles = height / tileHeight;
    const s32 tilecount = readInt(root, "tilecount");

    std::vector<s32> idToIx(tilecount, -1);
    if (root.contains("tiles")) {
        s32 ix = 0;
        for (const auto& tiledata : (root)["tiles"]) {
            s32 id = readInt(tiledata, "id");
            idToIx[id] = ix++;
        }
    }

    std::string maskPath = "";
    bool isAdditiveSpriteMask = false;
    if (root.contains("properties")) {
        for (JsonValue prop : root["properties"]) {
            if (prop["name"].getString() == "SpriteMask") {
                maskPath = prop["value"].getString();
            } else if (prop["name"].getString() == "SpriteMaskBlending") {
                std::string blending = prop["value"].getString();
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
        .margin = root["margin"].getInt(),
        .spacing = root["spacing"].getInt(),
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
    const Arc<JsonDoc> data = getWorldFile(mapfile);
    JsonValue root = **data;
    for (JsonValue propType : root["propertyTypes"]) {
        const std::string name = readString(propType, "name");
        const TiledDataType dtype = getDtype(propType["type"].getString());
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
                    const TiledDataType memberType = getDtype(member["type"].getString());
                    if (memberType == TiledDataType::Class) {
                        // this is a class, so we need to store its propertyType
                        std::string memberPropType = "";  // is possible that it's null
                        tryRead(member, "propertyType", &memberPropType);
                        ComponentFactory::memberTypes.insert({std::move(memberName), {memberType, std::move(memberPropType)}});
                        // TODO if it's an enum I also need to store the propertytype
                        // if the enum is stored as an int, memberType will be an int, but I'll also need propertytype
                        // and if the enum is stored as a string, memberType will be a string
                        // so I need to double check if memberPropType is in ComponentFactory2::propertyTypes
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
static Expected<Level::ParsedData> parseLevelInfo(const char* lvlFileName) {
    const Arc<JsonDoc> data = getMapFile(lvlFileName);
    JsonValue root = **data;
    Level::ParsedData lvlInfo;
    tryRead(root, "width", "height", &lvlInfo.sizeTiles);
    for (JsonValue property : root["properties"]) {
        std::string propType = readString(property, "propertytype");
        if (propType == "Map_MapInfo") {
            auto mapInfo = property["value"];
            tryRead(mapInfo, "AmbientLight", &lvlInfo.ambientLight);
            tryRead(mapInfo, "isWorldEntryPoint", &lvlInfo.isWorldEntryPoint);
            return lvlInfo;
        }
    }

    return Error(whal_format("Map_MapInfo property not found in level: {}", lvlFileName));
}

Corrade::Containers::Optional<Error> parseWorld(const char* mapfile, Scene& dstScene) {
    const Arc<JsonDoc> data = getWorldFile(mapfile);
    JsonValue root = **data;
    dstScene.self.setName(mapfile);

#ifndef NDEBUG
    std::string type = readString(root, "type");
    if (type != "world") {
        return Error("Not a world file");
    }
#endif

    dstScene.name = mapfile;
    for (JsonValue map : root["maps"]) {
        std::string filename = map["fileName"].getString();
        s32 x = readInt(map, "x");
        s32 y = readInt(map, "y");
        s32 width = readInt(map, "width");
        s32 height = readInt(map, "height");
        Expected<Level::ParsedData> eLvlInfo = parseLevelInfo(filename.c_str());
        if (eLvlInfo.isExpected()) {
            // NOTE: the world file stores map dimensions in PIXELS
            Level lvl = {
                filename, Vector2f(x, -y), Vector2f(width, height), eLvlInfo->sizeTiles, eLvlInfo->ambientLight, eLvlInfo->isWorldEntryPoint};
            if (lvl.isWorldEntryPoint) {
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
        return Error("Scene is not valid (didn't find world entry point)");
    }

    return NullOpt;
}

Transform getMapTransform(Vector2i mapPosition, Vector2i entitySize, ecs::Entity parent) {
    return TransformBuilder(parent).translate(getMapTranslation(mapPosition, entitySize)).build();
}

// converts relative map position (where origin is top-left) to relative world position (origin is the middle)
Vector2f getMapTranslation(Vector2i mapPosition, Vector2i entitySize) {
    return Vector2f(mapPosition.x + entitySize.x * 0.5 - PIXELS_PER_TILE / 2, -mapPosition.y - entitySize.y / 2 + PIXELS_PER_TILE / 2);
}

// MAP LOADING STUFF

const Arc<JsonDoc> getWorldFile(std::string_view mapFile) {
    const auto fullPath = whal_format("{}/{}", MAP_DIR, mapFile);
    return S_MAP_MANAGER.readData(fullPath.c_str());
}

const Arc<JsonDoc> getMapFile(std::string_view mapFile) {
    // const auto fullPath = whal_format("{}/exports/{}", MAP_DIR, mapFile);
    const auto fullPath = whal_format("{}/{}", MAP_DIR, mapFile);
    return S_MAP_MANAGER.readData(fullPath.c_str());
}

// TEMPLATE STUFF

const Arc<JsonValue> getTemplate(std::string_view templateFile) {
    const std::string fullPath = whal_format("{}/{}", MAP_DIR, templateFile);
    // create a new Arc because we're creating a new reference to the "object" field
    return Arc<JsonValue>::New((**S_TEMPLATE_MANAGER.readData(fullPath.c_str()))["object"]);
}

std::string getTypeFromTemplate(const std::string& templateFile) {
    const Arc<JsonValue> prefabData = getTemplate(templateFile);
    std::string objType = "";
    tryRead(*prefabData, "type", &objType);
    return objType;
}

}  // namespace whal
