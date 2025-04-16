#pragma once

#include <raylib.h>
#include <string>

#include "Components/Relationships.h"
#include "Util/Memory/Arc.h"
#include "Util/STL_reduce.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

struct TileMapInfo;
struct TileMap;
struct TileSet;

struct Scene {
    std::string name;
    std::vector<TileMapInfo> allLevels;
    std::vector<Arc<TileMap>> loadedLevels;
    stl::Map<std::string, Arc<TileSet>> tilesets;
    Vector2f startPos;
    s32 startLevelIx = -1;
    ecs::Entity self;

    void update();
    void unload();
    bool isValid() const;

    void setStartLevelIx(s32 ix);
    const TileMapInfo& getStartLevel() const;
    const TileMapInfo& getLevelAt(Vector2i worldPosition) const;
    Vector2i getClosestPositionInBounds(Vector2i worldPosition) const;

    void loadLevel(const TileMapInfo& level);
    void unloadAndRemoveLevel(TileMap& level);
    TileMap* tryGetLoadedLevel(const TileMapInfo& level);
    TileMap& getLoadedLevelAt(Vector2i worldPosition);      // loads level if not already loaded
    TileMap& getLoadedLevel(const TileMapInfo& level);      // loads level if not already loaded
    TileMap& getLoadedLevel(const std::string& levelPath);  // loads level if not already loaded
    TileMap& loadAndGetFirstLevel();                        // loads level if not already loaded
    bool isLevelLoaded(const TileMapInfo& level) const;
};

}  // namespace whal
