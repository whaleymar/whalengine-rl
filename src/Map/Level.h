#pragma once

#include <set>
#include <string>

#include "CorradeOptional.h"

#include "whalECS/src/Expected.h"

#include "Components/Relationships.h"
#include "Util/Vector.h"

typedef struct Color Color;

namespace whal {

struct TileMap;
class IGame;

namespace Map {
void registerGame(IGame* const pGame);
}

enum class LevelLighting { Dark, Normal, Dim };
Color getLightColor(LevelLighting);

struct Level {
    struct LevelInfo {
        bool isWorldEntryPoint = false;
        LevelLighting lighting = LevelLighting::Normal;
    };
    std::string filepath;     // used for level comparisons
    Vector2f worldPosOrigin;  // top left
    Vector2f size;
    LevelInfo lvlInfo;

    bool operator==(const Level& other) const { return filepath == other.filepath; }
};

struct ActiveLevel : public Level {
    std::set<ecs::Entity> childEntities;
    std::vector<ecs::Entity> objects;
    Vector2i worldOffsetPixels;

    Corrade::Containers::Optional<Follow> cameraFollow;
    Vector2i cameraFocalPoint;
    Vector2i initialSpawnPoint;
    std::vector<Vector2i> spawnPoints;

    void activateObjects();
    void deactivateObjects();
};

struct Scene {
    std::string name;
    std::vector<Level> allLevels;
    std::vector<ActiveLevel> loadedLevels;
    Vector2f startPos;
    std::set<ecs::Entity> childEntities;
    s32 startLevelIx = -1;
    Vector2i initialSpawnPoint;

    bool isValid() const;
    Corrade::Containers::Optional<Error> setStartLevelIx(s32 ix);
    Level getStartLevel() const;
    Expected<ActiveLevel*> loadAndGetFirstLevel();
    Vector2i getClosestSpawnPoint(ActiveLevel& activeLevel, Vector2i position);
    Corrade::Containers::Optional<Level> getLevelAt(Vector2i worldPosition) const;
    Vector2i getClosestPositionInBounds(Vector2i worldPosition) const;
    Expected<ActiveLevel*> getLoadedLevel(Level level);
};

Corrade::Containers::Optional<Error> loadLevel(const Level level);
void unloadAndRemoveLevel(ActiveLevel& level);
void unloadLevel(ActiveLevel& level);
void makeCollisionMesh(std::vector<std::vector<s32>>& collisionGrid, ActiveLevel& lvl);

}  // namespace whal
