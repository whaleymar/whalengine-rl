#pragma once

#include <vector>

#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Util/Optional.h"
#include "Util/STL_reduce.h"
#include "Util/Types.h"
#include "Util/Vector.h"
#include "whalECS/src/Expected.h"

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
Optional<Error> parseWorld(const char* mapfile, Scene& dstScene);
Transform getMapTransform(Vector2i mapPosition, Vector2i entitySize, ecs::Entity parent);
Vector2f getMapTranslation(Vector2i mapPosition, Vector2i entitySize);
const TileSet& getTileSet(const TileMap& map, s32 blockId);
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
    return TileInfo{
        .gid = static_cast<s32>(tileMask & 0x0FFFFFFF),  // Mask out the upper 4 bits to get the ID
        .isFlipH = (tileMask & 0x80000000) > 0,          // Check if the 32nd bit is on
        .isFlipY = (tileMask & 0x40000000) > 0,          // Check if the 31st bit is on
        .isRotate = (tileMask & 0x20000000) > 0,         // Check if the 30th bit is on
    };
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

// TODO i would like to merge this with ActiveLevel.
// ActiveLevel would need the tilesets and spriteCache variables, and everything holding a shared ptr to
// TileMap would need to point to ActiveLevel instead.
struct TileMap {
    // loads tile layers and objects as entities & adds them as children of the level
    static void load(const char* file, ActiveLevel& level);

    s32 widthTiles;
    s32 heightTiles;
    s32 tileSize;

    std::vector<TileSet> tilesets;
    stl::Map<u32, TileRenderInfo> spriteCache;  // key is GID
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
