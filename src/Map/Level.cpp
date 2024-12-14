#include "Level.h"

#include "Components/Tile.h"
#include "GameComponents/Respawn.h"
#include "IGame.h"
#include "Physics/CollisionLayer.h"

#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Entities/Block.h"

#include "Sys/System.h"
#include "Tiled.h"
#include "Util/Print.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

#define NULLOPT Corrade::Containers::NullOpt;

namespace whal {

void ActiveLevel::activateObjects() {
    for (auto entity : objects) {
        entity.activate();
    }
}

void ActiveLevel::deactivateObjects() {
    for (auto entity : objects) {
        entity.deactivate();
    }
}

bool Scene::isValid() const {
    return startLevelIx >= 0;
}

Corrade::Containers::Optional<Error> Scene::setStartLevelIx(s32 ix) {
    if (startLevelIx != -1) {
        return Error("Start level has already been set for scene");
    }
    startLevelIx = ix;
    return NULLOPT;
}

Level Scene::getStartLevel() const {
    assert(isValid());
    return allLevels[startLevelIx];
}

Expected<ActiveLevel*> Scene::loadAndGetFirstLevel() {
    auto eActiveLevel = getLoadedLevel(getStartLevel());
    return eActiveLevel;
}

Corrade::Containers::Optional<Level> Scene::getLevelAt(Vector2i worldPos) const {
    Vector2f worldPosF = worldPos.as<f32>();
    for (Level lvl : allLevels) {
        if (worldPosF.x >= lvl.worldPosOrigin.x && worldPosF.x < (lvl.worldPosOrigin.x + lvl.size.x) && worldPosF.y < lvl.worldPosOrigin.y &&
            worldPosF.y >= (lvl.worldPosOrigin.y - lvl.size.y)) {
            return lvl;
        }
    }
    return NULLOPT;
}

Expected<ActiveLevel*> Scene::getLoadedLevelAt(Vector2i worldPos) {
    auto lvlOpt = getLevelAt(worldPos);
    if (!lvlOpt) {
        return Error("No level at position");
    }

    return getLoadedLevel(*lvlOpt);
}

Vector2i Scene::getClosestPositionInBounds(Vector2i worldPos) const {
    s32 minDistance = 999999;
    Vector2i closestPosition;
    for (Level lvl : allLevels) {
        const AABB lvlBox((lvl.worldPosOrigin + lvl.size * Vector2f(0.5, -0.5)).as<s32>(), (lvl.size * 0.5).as<s32>());

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

Expected<ActiveLevel*> Scene::getLoadedLevel(Level level) {
    auto it = ecs::whal_find(loadedLevels.begin(), loadedLevels.end(), level);
    if (it != loadedLevels.end()) {
        return &(*it);
    }

    // load it
    auto errOpt = loadLevel(level);
    if (errOpt) {
        return *errOpt;
    }
    ActiveLevel* result = &loadedLevels[loadedLevels.size() - 1];
    assert(result->filepath == level.filepath && "Last active level doesn't match passed arg");
    return result;
}

Corrade::Containers::Optional<Error> loadLevel(const Level level) {
    Vector2i worldOffsetPixels = Transform::pixels(level.worldPosOrigin.x, level.worldPosOrigin.y - level.size.y).position;
    ActiveLevel lvl = {level, {}, {}, worldOffsetPixels, {}, {}, {}};
    TileMap::load(level.filepath.c_str(), lvl);
    print("loaded map: ", level.filepath);
    System::getGame().getScene().loadedLevels.push_back(lvl);

    return NULLOPT;
}

void unloadAndRemoveLevel(ActiveLevel& level) {
    // remove from Scene's list of loaded levels first,
    // so the EntityKilled listener doesn't mutate the list we're iterating
    // also copy it so erasing it doesn't invalidate our pointer

    ActiveLevel copy = level;
    Scene& scene = System::getGame().getScene();
    for (auto it = scene.loadedLevels.begin(); it != scene.loadedLevels.end(); ++it) {
        auto& lvl = *it;
        if (lvl == level) {
            scene.loadedLevels.erase(it);
            break;
        }
    }
    unloadLevel(copy);
}

void unloadLevel(ActiveLevel& level) {
    std::set<ecs::Entity> toKill = std::move(level.childEntities);
    for (auto entity : toKill) {
        if (entity.has<Respawn>()) {
            entity.remove<Respawn>();
        }
        entity.kill();
    }
    print("unloaded level:", level.filepath);
}

}  // namespace whal
