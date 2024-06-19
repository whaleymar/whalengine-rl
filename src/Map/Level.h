#pragma once

#include <set>
#include <string>

#include "CorradeOptional.h"

#include "whalECS/src/Expected.h"

#include "ECS/Relationships.h"
#include "Util/Vector.h"

namespace whal {

struct TileMap;

struct Level {
    struct LevelInfo {
        bool isWorldEntryPoint = false;
    };
    std::string filepath;           // used for level comparisons
    Vector2f worldPosOriginTexels;  // top left
    Vector2f sizeTexels;
    LevelInfo lvlInfo;

    bool operator==(const Level& other) const { return filepath == other.filepath; }
};

struct ActiveLevel : public Level {
    std::string name;
    std::set<ecs::Entity> childEntities;
    Vector2i worldOffsetPixels;

    Corrade::Containers::Optional<Follow> cameraFollow;
    Vector2i cameraFocalPoint;
    Vector2i spawnPoint;  // TODO i want to support multiple of these in the map data & have them update based on where the player entered the level
                          // from / update them with triggers. I have a LEVEL_ENTER event which I can use once I have multiple spawn points
};

struct Scene {
    std::string name;
    std::vector<Level> allLevels;
    std::vector<ActiveLevel> loadedLevels;
    Vector2f startPos;
    std::set<ecs::Entity> childEntities;
    s32 startLevelIx = -1;

    bool isValid() const;
    Corrade::Containers::Optional<Error> setStartLevelIx(s32 ix);
    Level getStartLevel() const;
    Vector2i getStartPosition();
    Corrade::Containers::Optional<Level> getLevelAt(Vector2f worldPosTexels) const;
    Expected<ActiveLevel*> getLoadedLevel(Level level);
};

Corrade::Containers::Optional<Error> loadLevel(const Level level);
void unloadAndRemoveLevel(ActiveLevel& level);
void unloadLevel(ActiveLevel& level);
void makeCollisionMesh(std::vector<std::vector<s32>>& collisionGrid, ActiveLevel& lvl);

}  // namespace whal
