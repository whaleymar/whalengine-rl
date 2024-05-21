#pragma once

#include <set>

#include "ECS/Relationships.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

class SolidCollider;
struct TileMap;

struct Level {
    std::string filepath;           // used for level comparisons
    Vector2f worldPosOriginTexels;  // top left
    Vector2f sizeTexels;

    bool operator==(const Level& other) const { return filepath == other.filepath; }
};

struct ActiveLevel : public Level {
    std::string name;
    std::set<ecs::Entity> childEntities;
    Vector2i worldOffsetPixels;

    std::optional<Follow> cameraFollow;
    Vector2i cameraFocalPoint;
};

struct Scene {
    std::string name;
    std::vector<Level> allLevels;
    std::vector<ActiveLevel> loadedLevels;
    Vector2f startPos;
    std::set<ecs::Entity> childEntities;

    std::optional<Level> getLevelAt(Vector2f worldPosTexels);
    Expected<ActiveLevel*> getLoadedLevel(Level level);
};

std::optional<Error> loadLevel(const Level level);
void unloadAndRemoveLevel(ActiveLevel& level);
void unloadLevel(ActiveLevel& level);
void removeEntityFromLevel(ecs::Entity entity);
void makeCollisionMesh(std::vector<std::vector<s32>>& collisionGrid, ActiveLevel& lvl);

}  // namespace whal
