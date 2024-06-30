#pragma once

#include <vector>

#include "CorradeOptional.h"
#include "CorradePointer.h"

#include "Util/Vector.h"
#include "whalECS/src/Expected.h"

#include "Gfx/Depth.h"
#include "Physics/Material.h"
#include "Util/Types.h"

namespace whal {

struct Frame;
struct TileMap;
struct TileSet;
struct Scene;
struct Transform2D;
struct ActiveLevel;

Expected<Frame> getTileFrame(const TileMap& map, s32 blockIx);
Corrade::Containers::Optional<Error> parseMapProject(const char* projectfile);
Corrade::Containers::Optional<Error> parseWorld(const char* mapfile, Scene& dstScene);
Transform2D getTransformFromMapPosition(Vector2i mapCenterPositionTexels, Vector2i dimensionsTexels, const ActiveLevel& level, bool isPoint);
const TileSet* getTileSet(const TileMap& map, s32 blockId);

struct LayerData {
    Depth depth;
    f32 parallax = 1.0;
};

struct EntityMapData {
    Vector2i position;  // top left of tile
    Vector2i dimensionsTexels;
    s32 id;
    bool isPoint;
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
    s32 tileWidthTexels;
    s32 tileHeightTexels;
    s32 widthTiles;
    s32 heightTiles;
    s32 margin;
    s32 spacing;
    std::string fileName;
    std::string spriteFileName;
    std::vector<WorldMaterial> materials;
};

struct TileMap {
    static TileMap parse(const char* file, ActiveLevel& level);

    std::string name;
    s32 widthTiles;
    s32 heightTiles;
    s32 tileSize;

    std::vector<TileLayer> layers;
    std::vector<TileSet> tilesets;
};

}  // namespace whal
