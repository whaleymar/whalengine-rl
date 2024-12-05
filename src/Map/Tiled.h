#pragma once

#include <vector>

#include "CorradeOptional.h"
#include "CorradePointer.h"
#include "json_fwd.hpp"

#include "Util/Vector.h"
#include "whalECS/src/Expected.h"

#include "Gfx/Color.h"
#include "Gfx/Depth.h"
#include "Util/Types.h"

namespace rl {
typedef struct Color Color;
}

namespace whal {

struct Frame;
struct TileMap;
struct TileSet;
struct Scene;
struct Transform;
struct ActiveLevel;
class Shape;
struct LoadContext;

namespace ecs {
class Entity;
}

Expected<Frame> getTileFrame(const TileMap& map, s32 blockIx);
void parseMapProject(const char* projectfile);
Corrade::Containers::Optional<Error> parseWorld(const char* mapfile, Scene& dstScene);
Transform getTransformFromMapPosition(Vector2i mapCenter, Vector2i size, const ActiveLevel& level, bool isPoint);
const TileSet& getTileSet(const TileMap& map, s32 blockId);
Vector2i getObjectSize(const nlohmann::json& objectData);
void clearMapCache();

// JSON PARSING UTILITY FUNCTIONS
s32 readInt(const nlohmann::json& json, std::string_view key);
s32 readFloat(const nlohmann::json& json, std::string_view key);
Vector2i readVector2i(const nlohmann::json& json, const char* xKey = "x", const char* yKey = "y");
Vector2f readVector2f(const nlohmann::json& json, const char* xKey = "x", const char* yKey = "y");
bool readBool(const nlohmann::json& data, std::string_view key);
std::string readString(const nlohmann::json& json, std::string_view key);
Color readColor(const std::string& hexString);
Depth readDepth(const std::string& depthString);
bool tryReadInt(const nlohmann::json& data, std::string_view key, s32* dst);
bool tryReadFloat(const nlohmann::json& data, std::string_view key, f32* dst);
bool tryReadVector2i(const nlohmann::json& data, std::string_view xKey, std::string_view yKey, Vector2i* dst);
bool tryReadVector2f(const nlohmann::json& data, std::string_view xKey, std::string_view yKey, Vector2f* dst);
bool tryReadBool(const nlohmann::json& data, std::string_view key, bool* dst);
bool tryReadString(const nlohmann::json& data, std::string_view key, std::string* dst);
bool tryReadColor(const nlohmann::json& data, std::string_view key, Color* dst);
bool tryReadDepth(const nlohmann::json& data, std::string_view key, Depth* dst);

// different API:
Shape readShape(const LoadContext& ctx, ecs::Entity entity, std::string_view key, Vector2i* dstOffset = nullptr);
Shape getDefaultShape(const LoadContext& ctx, ecs::Entity entity);  // uses shape of Tiled object instead of a Property
bool tryReadShape(const LoadContext& ctx, ecs::Entity entity, std::string_view key, Shape* dst, Vector2i* dstOffset = nullptr);
Shape readShapeOrDefault(const LoadContext& ctx, ecs::Entity entity, std::string_view key,
                         Vector2i* dstOffset = nullptr);  // tries to get a custom shape, returns default shape if none present

struct LayerData {
    Depth depth;
    f32 parallax = 1.0;
};

struct EntityMapData {
    Vector2i position;  // top left of tile
    Vector2i size;
    s32 id;
    bool isPoint;
    bool isParsingTemplate = false;
};

struct TileLayer {
    std::string name;
    s32 width;
    s32 height;
    LayerData metadata;
    std::vector<s32> data;
};

struct TileSet {
    s32 firstgid;
    s32 tilecount;
    s32 tileWidth;
    s32 tileHeight;
    s32 widthTiles;
    s32 heightTiles;
    s32 margin;
    s32 spacing;
    std::string fileName;
    std::string spriteFileName;
    std::vector<s32> tileIDToIndex;

    void addTileComponents(ecs::Entity entity, s32 tileID, const ActiveLevel& level, LayerData layerData, Vector2i mapPosition) const;
};

struct TileMap {
    static TileMap parse(const char* file, ActiveLevel& level);

    s32 widthTiles;
    s32 heightTiles;
    s32 tileSize;

    std::vector<TileLayer> layers;
    std::vector<TileSet> tilesets;
};

// metadata about a Tiled Property (ie a custom type)
// TODO parse this info from the Tiled .tiled-project file into a map & pass it into the loader functions
struct PropertyType {
    enum class DataType {
        Int,
        String,
        Float,
        Bool,
        Object,
        Enum,
        Color,
        Class,
    };

    struct EnumInfo {
        bool isString;
        bool isFlags;
    };

    std::string name;
    DataType dtype;
    EnumInfo enumInfo;  // only defined when dtype == Enum
};

}  // namespace whal
