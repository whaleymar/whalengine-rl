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
    // TODO private this and friend Scene?
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
    std::vector<Arc<ActiveLevel>> loadedLevels;  // TODO Box so loading/unloading others doesn't cause errors
    stl::Map<std::string, Arc<TileSet>> tilesets;
    Vector2f startPos;
    s32 startLevelIx = -1;
    ecs::Entity self;

    void update();
    void unload();
    bool isValid() const;

    void setStartLevelIx(s32 ix);
    const Level& getStartLevel() const;
    const Level& getLevelAt(Vector2i worldPosition) const;
    Vector2i getClosestPositionInBounds(Vector2i worldPosition) const;

    void loadLevel(const Level& level);
    void unloadAndRemoveLevel(ActiveLevel& level);
    ActiveLevel* tryGetLoadedLevel(const Level& level);
    // TODO return references since these aren't nullable?
    ActiveLevel* getLoadedLevelAt(Vector2i worldPosition);      // loads level if not already loaded
    ActiveLevel* getLoadedLevel(const Level& level);            // loads level if not already loaded
    ActiveLevel* getLoadedLevel(const std::string& levelPath);  // loads level if not already loaded
    ActiveLevel* loadAndGetFirstLevel();                        // loads level if not already loaded
    bool isLevelLoaded(const Level& level) const;
};

}  // namespace whal
