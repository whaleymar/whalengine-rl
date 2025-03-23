#include "Level.h"

#include "Components/Collider.h"
#include "Components/Draw.h"
#include "Components/Tags.h"
#include "Components/Transform.h"

#include "Systems/ColliderSystem.h"

#include "Gfx/Coordinates.h"
#include "IGame.h"
#include "Sys/System.h"
#include "Tiled.h"
#include "Util/Print.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

#define NULLOPT Corrade::Containers::NullOpt;

namespace whal {

AABB Level::getBoundingBox() const {
    return AABB((position + size * Vector2f(0.5, -0.5)).as<s32>(), (size * 0.5).as<s32>());
}

Vector2i Level::worldPositionToTileClamped(Vector2i worldPosition) const {
    return ((worldToTileCoords(worldPosition) - worldToTileCoords(position.as<s32>())) * Vector2i(1, -1))
        .clamp({0, 0}, {sizeTiles.x - 1, sizeTiles.y - 1});
}

ActiveLevel::ActiveLevel(const Level& base, Vector2i worldOffset, ecs::Entity parent) : Level(base) {
    self = parent.createChild(base.filepath.c_str());
    self.set(TransformBuilder(self.get<Transform>()).translate(worldOffset.as<f32>()).build());
    TileMap::load(base.filepath.c_str(), *this);
}

static ecs::Entity findChild(ecs::Entity parent, const std::string& name) {
    for (const ecs::Entity& child : parent.children()) {
        if (name == child.name()) {
            return child;
        }

        if (child.has<TiledObjectLayer>()) {
            ecs::Entity maybe = findChild(child, name);
            if (maybe.isValid()) {
                return maybe;
            }
        }
    }
    return ecs::Entity{};
}

ecs::Entity ActiveLevel::getChild(const std::string& name) {
    return findChild(self, name);
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
        if (worldPosF.x >= lvl.position.x && worldPosF.x < (lvl.position.x + lvl.size.x) && worldPosF.y < lvl.position.y &&
            worldPosF.y >= (lvl.position.y - lvl.size.y)) {
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
    for (const Level& lvl : allLevels) {
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

Expected<ActiveLevel*> Scene::getLoadedLevel(const std::string& levelPath) {
    for (const auto& aLvl : allLevels) {
        if (aLvl.filepath == levelPath) {
            return getLoadedLevel(aLvl);
        }
    }
    return Error(whal_format("Level not found: {}", levelPath));
}

void Scene::update() {
    // RESEARCH extension: navigation grid stores a u16 with collision layer information
    for (ActiveLevel& lvl : loadedLevels) {
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
}

Corrade::Containers::Optional<Error> loadLevel(const Level level) {
    Vector2i worldOffset(level.position.x, level.position.y);
    Scene& scene = System::getGame().getScene();
    scene.loadedLevels.emplace_back(ActiveLevel(level, worldOffset, scene.self));
    print("loaded map: ", level.filepath);

    return NULLOPT;
}

void unloadAndRemoveLevel(ActiveLevel& level) {
    // remove from Scene's list of loaded levels first,
    // so the EntityKilled listener doesn't mutate the list we're iterating
    // also copy it so erasing it doesn't invalidate our pointer

    print("unloading level from scene: ", level.filepath);
    ecs::Entity e = level.self;  // copy entity before `level` is deleted (and pointer is invalidated)
    Scene& scene = System::getGame().getScene();
    for (auto it = scene.loadedLevels.begin(); it != scene.loadedLevels.end(); ++it) {
        auto& lvl = *it;
        if (lvl == level) {
            scene.loadedLevels.erase(it);
            break;
        }
    }

    const bool wasPaused = System::isQuietPaused();
    System::setQuietPaused(true);
    e.kill();
    System::setQuietPaused(wasPaused);
}

}  // namespace whal
