#pragma once

#include <vector>

#include "CorradeOptional.h"

#include "whalECS/src/Expected.h"

#include "ECS/Transform.h"
#include "Map/ComponentFactory.h"
#include "Map/Level.h"
#include "Physics/Material.h"
#include "Util/Types.h"

namespace whal {

struct Frame;
struct TileMap;
struct TileSet;
struct Scene;

Expected<Frame> getTileFrame(const TileMap& map, s32 blockIx);
Corrade::Containers::Optional<Error> parseMapProject(const char* projectfile);
Corrade::Containers::Optional<Error> parseWorld(const char* mapfile, Scene& dstScene);
Transform2D getTransformFromMapPosition(Vector2i mapCenterPositionTexels, Vector2i dimensionsTexels, ActiveLevel& level, bool isPoint);
const TileSet* getTileSet(const TileMap& map, s32 blockId);

struct LayerData {
    Depth depth;
    f32 parallax = 1.0;
};

struct TileLayer {
    std::string name;
    s32 width;
    s32 height;
    LayerData metadata;
    std::unique_ptr<s32[]> data;
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

    static inline ComponentFactory componentFactory;

    std::string name;
    s32 widthTiles;
    s32 heightTiles;
    s32 tileSize;

    std::vector<TileLayer> layers;
    std::vector<TileSet> tilesets;
};

}  // namespace whal
