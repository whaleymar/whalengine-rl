#include "Scene.h"

#include "Components/Collider.h"
#include "Components/Draw.h"
#include "Components/Tags.h"
#include "Components/Transform.h"

#include "Sys/Time.h"
#include "Systems/ColliderSystem.h"

#include "IGame.h"
#include "Sys/System.h"
#include "Tiled.h"
#include "Util/Print.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

bool Scene::isValid() const {
    return startLevelIx >= 0;
}

bool Scene::isLevelLoaded(const TileMapInfo& level) const {
    for (const Arc<TileMap>& lvl : loadedLevels) {
        if (*lvl == level) {
            return true;
        }
    }
    return false;
}

void Scene::setStartLevelIx(s32 ix) {
    assert(startLevelIx == -1 && "Start level has already been set for scene");
    startLevelIx = ix;
}

const TileMapInfo& Scene::getStartLevel() const {
    assert(isValid());
    return allLevels[startLevelIx];
}

TileMap& Scene::loadAndGetFirstLevel() {
    return getLoadedLevel(getStartLevel());
}

const TileMapInfo& Scene::getLevelAt(Vector2i worldPos) const {
    Vector2f worldPosF = worldPos.as<f32>();
    for (const TileMapInfo& lvl : allLevels) {
        if (worldPosF.x >= lvl.position.x && worldPosF.x < (lvl.position.x + lvl.size.x) && worldPosF.y < lvl.position.y &&
            worldPosF.y >= (lvl.position.y - lvl.size.y)) {
            return lvl;
        }
    }
    assert(false && "Level not found");
}

TileMap& Scene::getLoadedLevelAt(Vector2i worldPos) {
    return getLoadedLevel(getLevelAt(worldPos));
}

Vector2i Scene::getClosestPositionInBounds(Vector2i worldPos) const {
    s32 minDistance = 999999;
    Vector2i closestPosition;
    for (const TileMapInfo& lvl : allLevels) {
        const AABB lvlBox = lvl.getBoundingBox();

        const auto delta = worldPos - lvlBox.getPosition();
        const auto half = lvlBox.getHalf();
        const auto closestPoint = lvlBox.getPosition() + Vector2i(math::clamp(delta.x, -half.x, half.x), math::clamp(delta.y, -half.y, half.y));

        s32 distance = (closestPoint - worldPos).len();
        if (distance < minDistance) {
            minDistance = distance;
            closestPosition = closestPoint;
        }
    }

    return closestPosition;
}

TileMap* Scene::tryGetLoadedLevel(const TileMapInfo& level) {
    for (Arc<TileMap>& lvl : loadedLevels) {
        if (*lvl == level) {
            return lvl.get();
        }
    }
    return nullptr;
}

TileMap& Scene::getLoadedLevel(const TileMapInfo& level) {
    TileMap* result = tryGetLoadedLevel(level);
    if (result) {
        return *result;
    }

    // need to load it
    loadLevel(level);
    result = loadedLevels[loadedLevels.size() - 1].get();
    assert(result->filepath == level.filepath && "Last active level doesn't match passed arg");
    return *result;
}

TileMap& Scene::getLoadedLevel(const std::string& levelPath) {
    for (const auto& aLvl : allLevels) {
        if (aLvl.filepath == levelPath) {
            return getLoadedLevel(aLvl);
        }
    }
    assert(false && "Level not found");
}

void Scene::update() {
    // RESEARCH extension: navigation grid stores a u16 with collision layer information
    for (Arc<TileMap>& pLvl : loadedLevels) {
        TileMap& lvl = *pLvl;
        lvl.navGridDynamic = std::vector<std::vector<u32>>(lvl.sizeTiles.x, std::vector<u32>(lvl.sizeTiles.y, 0));
        AABB lvlBox = lvl.getBoundingBox();
        std::vector<ecs::Entity> colliders = ColliderSystem::query(lvlBox);
        for (ecs::Entity e : colliders) {
            if (e.has<TileTag>()) {
                continue;
            }
            const Collider& collider = e.get<Collider>();
            if (collider.getBodyType() == PhysicsBody::Feather) {
                continue;
            }

            Vector2i topLeftTile = lvl.worldPositionToTileClamped(collider.getShape().getPositionEdge(Vector2i(-1, 1)));
            // for bottom right, get the tile that's 1px up and left
            Vector2i bottomRightTile = lvl.worldPositionToTileClamped(collider.getShape().getPositionEdge(Vector2i(1, -1)) + Vector2i(-1, 1));
            for (s32 i = topLeftTile.x; i <= bottomRightTile.x; i++) {
                for (s32 j = topLeftTile.y; j <= bottomRightTile.y; j++) {
                    lvl.navGridDynamic[i][j] |= e.id();
                }
            }
        }
    }

    if (Time.getFrame() % 60 == 1) {
        // check if any tilesets can be unloaded
        // if it has a refcnt of 1 we hold the only reference
        auto& tsData = tilesets.getData();
        auto it = tsData.begin();
        while (it != tsData.end()) {
            if (it->value.use_count() == 1) {
                it = tsData.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void Scene::unload() {
    // setting quiet paused so system callbacks don't run? Dunno if this is still necessary
    const bool wasPaused = System::isQuietPaused();
    System::setQuietPaused(true);
    self.kill();
    startLevelIx = -1;
    allLevels.clear();
    loadedLevels.clear();
    tilesets.clear();

    World.killEntities();
    self = ecs::Entity{};  // invalidate scene root
    System::setQuietPaused(wasPaused);
}

void Scene::loadLevel(const TileMapInfo& level) {
    Vector2i worldOffset(level.position.x, level.position.y);
    Arc<TileMap> loaded = Arc<TileMap>(new TileMap(level, worldOffset, self));
    loadedLevels.emplace_back(loaded);
    print("loaded map: ", level.filepath);
}

void Scene::unloadAndRemoveLevel(TileMap& level) {
    print("unloading level from scene: ", level.filepath);
    level.self.kill();
    for (auto it = loadedLevels.begin(); it != loadedLevels.end(); ++it) {
        Arc<TileMap>& lvl = *it;
        if (*lvl == level) {
            loadedLevels.erase(it);
            break;
        }
    }
}

}  // namespace whal
