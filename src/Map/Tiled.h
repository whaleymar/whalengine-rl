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
Color parseColor(const std::string& hexString);
bool tryReadColor(const nlohmann::json& data, std::string_view key, Color* dst);
Depth parseDepth(const std::string& depthString);
bool tryReadDepth(const nlohmann::json& data, std::string_view key, Depth* dst);

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

}  // namespace whal
