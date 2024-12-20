#pragma once

#include <raylib.h>
#include <set>
#include <string>

#include "CorradeOptional.h"

#include "whalECS/src/Expected.h"

#include "Components/Relationships.h"
#include "Gfx/Color.h"
#include "Util/Vector.h"

namespace whal {

struct TileMap;

struct Level {
    struct MetaData {
        bool isWorldEntryPoint = false;
        Color ambientLight = Colors::White;
    };
    std::string filepath;     // used for level comparisons
    Vector2f worldPosOrigin;  // top left
    Vector2f size;
    MetaData meta;

    bool operator==(const Level& other) const { return filepath == other.filepath; }
};

struct ActiveLevel : public Level {
    std::set<ecs::Entity> childEntities;
    std::vector<ecs::Entity> objects;
    Vector2i worldOffset;

    Corrade::Containers::Optional<Follow> cameraFollow;
    Vector2i cameraFocalPoint;
    std::vector<std::vector<bool>> navGrid;  // true == no obstacle at tile

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

    bool isValid() const;
    Corrade::Containers::Optional<Error> setStartLevelIx(s32 ix);
    Level getStartLevel() const;
    Expected<ActiveLevel*> loadAndGetFirstLevel();
    Corrade::Containers::Optional<Level> getLevelAt(Vector2i worldPosition) const;
    Expected<ActiveLevel*> getLoadedLevelAt(Vector2i worldPosition);
    Vector2i getClosestPositionInBounds(Vector2i worldPosition) const;
    Expected<ActiveLevel*> getLoadedLevel(Level level);
};

Corrade::Containers::Optional<Error> loadLevel(const Level level);
void unloadAndRemoveLevel(ActiveLevel& level);
void unloadLevel(ActiveLevel& level);

}  // namespace whal
