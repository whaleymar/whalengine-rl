#pragma once

#include <vector>

#include "Components/Draw.h"
#include "Components/Transform.h"
#include "CorradeOptional.h"
#include "CorradePointer.h"
#include "json_fwd.hpp"

#include "whalECS/src/Expected.h"

#include "Util/STL_reduce.h"
#include "Util/Types.h"
#include "Util/Vector.h"

namespace rl {
typedef struct Color Color;
}

namespace whal {

struct TileMap;
struct TileSet;
struct Scene;
struct ActiveLevel;

struct TileRenderInfo {
    Sprite sprite;
    std::pair<f32, Facing> orient;
    bool isOccluder;
};

namespace ecs {
class Entity;
}

Expected<Sprite> getTileSprite(const TileMap& map, s32 blockIx);
void parseMapProject(const char* projectfile);
Corrade::Containers::Optional<Error> parseWorld(const char* mapfile, Scene& dstScene);
Transform getTransformFromMapPosition(Vector2i mapCenter, Vector2i size, const ActiveLevel& level, bool isPoint);
const TileSet& getTileSet(const TileMap& map, s32 blockId);
Vector2i getObjectSize(const nlohmann::json& objectData);
void clearMapCache();

struct EntityMapData {
    Vector2i position;  // top left of tile
    Vector2i size;
    s32 id;
    bool isPoint;
    bool isParsingTemplate = false;
};

struct TileInfo {
    s32 gid;
    bool isFlipH;
    bool isFlipY;
    bool isRotate;
};

inline TileInfo getTile(u32 tileMask) {
    TileInfo tile;
    tile.isFlipH = tileMask & 0x80000000;   // Check if the 32nd bit is on
    tile.isFlipY = tileMask & 0x40000000;   // Check if the 31st bit is on
    tile.isRotate = tileMask & 0x20000000;  // Check if the 30th bit is on
    tile.gid = tileMask & 0x0FFFFFFF;       // Mask out the upper 4 bits to get the ID
    return tile;
}

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
    std::string spriteMaskFileName;
    std::vector<s32> localIDToPropsIndex;
    bool isAdditiveSpriteMask = false;
};

struct TileMap {
    // loads tile layers and objects as entities & adds them as children of the level
    static void load(const char* file, ActiveLevel& level);

    s32 widthTiles;
    s32 heightTiles;
    s32 tileSize;

    std::vector<TileSet> tilesets;
    stl::Map<s32, TileRenderInfo> spriteCache;  // key is GID
};

// Supported data types in Tiled
enum class TiledDataType {
    Int,
    String,
    Float,
    Bool,
    Object,
    Enum,
    Color,
    Class,
};

// metadata about a Tiled Property (ie a custom type)
struct PropertyType {
    struct EnumInfo {
        bool isString;
        bool isFlags;
    };

    TiledDataType dtype;
    EnumInfo enumInfo;  // only defined when dtype == Enum
};

}  // namespace whal
