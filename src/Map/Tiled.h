#pragma once

#include <vector>

#include "Components/Draw.h"
#include "Components/Relationships.h"
#include "Components/Transform.h"
#include "ECS.h"
#include "Util/Memory/Arc.h"
#include "Util/Optional.h"
#include "Util/STL_reduce.h"
#include "Util/Types.h"
#include "Util/Vector.h"
#include "whalECS/src/Expected.h"

namespace rl {
typedef struct Color Color;
}

namespace whal {

class AABB;
struct TileSet;
struct TileSetRef;
struct Scene;

struct TileRenderInfo {
    Sprite sprite;
    std::pair<f32, Facing> orient;
    bool isOccluder;
};

namespace ecs {
class Entity;
}

void parseMapProject(const char* projectfile);
Optional<Error> parseWorld(const char* mapfile, Scene& dstScene);
Transform getMapTransform(Vector2i mapPosition, Vector2i entitySize, ecs::Entity parent);
Vector2f getMapTranslation(Vector2i mapPosition, Vector2i entitySize);
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

struct TileSetRef {
    s32 firstgid;
    Arc<TileSet> tileset;
};

struct TileMapInfo {
    struct ParsedData {
        Vector2i sizeTiles;
        Color ambientLight = Colors::White;
        bool isWorldEntryPoint = false;
    };

    std::string filepath;  // used for level comparisons
    Vector2f position;     // top left
    Vector2f size;         // in pixels
    Vector2i sizeTiles;
    Color ambientLight = Colors::White;
    bool isWorldEntryPoint = false;

    AABB getBoundingBox() const;
    Vector2i worldPositionToTileClamped(Vector2i worldPosition) const;
    bool operator==(const TileMapInfo& other) const { return filepath == other.filepath; }
};

struct TileMap : public TileMapInfo {
    friend Scene;

    ecs::Entity self;
    Optional<Follow> cameraFollow;
    Vector2i cameraFocalPoint;
    std::vector<std::vector<u8>> navGrid;  // 1 == no obstacle at tile. Tile geometry only.

    // Holds IDs of collider entities on the map (excluding tiles).
    // Entity IDs are bitwise OR'd if multiple entities are on the tile.
    std::vector<std::vector<u32>> navGridDynamic;
    std::vector<TileSetRef> tilesets;
    stl::Map<u32, TileRenderInfo> spriteCache;  // key is GID

    ecs::Entity getChild(const std::string& name);
    const TileSetRef& getTileSet(s32 blockId) const;
    Sprite getTileSprite(s32 blockId) const;

private:
    TileMap(const TileMapInfo& base, Vector2i worldOffset, ecs::Entity parent);
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
