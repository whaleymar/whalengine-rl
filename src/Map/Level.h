#pragma once

#include <raylib.h>
#include <string>

#include "CorradeOptional.h"

#include "whalECS/src/Expected.h"

#include "Components/Relationships.h"
#include "Gfx/Color.h"
#include "Util/Vector.h"

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

    Corrade::Containers::Optional<Follow> cameraFollow;
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
    std::vector<ActiveLevel> loadedLevels;
    Vector2f startPos;
    s32 startLevelIx = -1;
    ecs::Entity self;

    bool isValid() const;
    Corrade::Containers::Optional<Error> setStartLevelIx(s32 ix);
    Level getStartLevel() const;
    Expected<ActiveLevel*> loadAndGetFirstLevel();
    Corrade::Containers::Optional<Level> getLevelAt(Vector2i worldPosition) const;
    Expected<ActiveLevel*> getLoadedLevelAt(Vector2i worldPosition);
    Vector2i getClosestPositionInBounds(Vector2i worldPosition) const;
    Expected<ActiveLevel*> getLoadedLevel(Level level);
    Expected<ActiveLevel*> getLoadedLevel(const std::string& levelPath);
    void update();
};

Corrade::Containers::Optional<Error> loadLevel(const Level level);
void unloadAndRemoveLevel(ActiveLevel& level);

}  // namespace whal
