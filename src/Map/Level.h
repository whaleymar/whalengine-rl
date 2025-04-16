#pragma once

#include <raylib.h>
#include <string>

#include "Components/Relationships.h"
#include "Gfx/Color.h"
#include "Map/Tiled.h"
#include "Util/Memory/Arc.h"
#include "Util/Optional.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

class AABB;

struct Level {
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
    bool operator==(const Level& other) const { return filepath == other.filepath; }
};

struct ActiveLevel : public Level {
    ActiveLevel(const Level& base, Vector2i worldOffset, ecs::Entity parent);

    ecs::Entity self;

    Optional<Follow> cameraFollow;
    Vector2i cameraFocalPoint;
    std::vector<std::vector<u8>> navGrid;  // 1 == no obstacle at tile. Tile geometry only.

    // Holds IDs of collider entities on the map (excluding tiles).
    // Entity IDs are bitwise OR'd if multiple entities are on the tile.
    std::vector<std::vector<u32>> navGridDynamic;

    ecs::Entity getChild(const std::string& name);
};

struct Scene {
    std::string name;
    std::vector<Level> allLevels;
    std::vector<ActiveLevel> loadedLevels;  // TODO Box so loading/unloading others doesn't cause errors
    stl::Map<std::string, Arc<TileSet>> tilesets;
    Vector2f startPos;
    s32 startLevelIx = -1;
    ecs::Entity self;

    bool isValid() const;
    void setStartLevelIx(s32 ix);
    Level getStartLevel() const;
    ActiveLevel* loadAndGetFirstLevel();
    Level getLevelAt(Vector2i worldPosition) const;
    ActiveLevel* getLoadedLevelAt(Vector2i worldPosition);
    Vector2i getClosestPositionInBounds(Vector2i worldPosition) const;
    ActiveLevel* getLoadedLevel(Level level);
    ActiveLevel* getLoadedLevel(const std::string& levelPath);
    void update();
    void unload();
    void loadLevel(const Level level);
    void unloadAndRemoveLevel(ActiveLevel& level);
};

}  // namespace whal
