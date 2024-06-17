#include "Level.h"

#include "Settings.h"

#include "ECS/Collision.h"
#include "ECS/Draw.h"
#include "ECS/Entities/Block.h"
#include "ECS/Name.h"
#include "ECS/Transform.h"

#include "Game.h"
#include "Gfx/Texture.h"
#include "Systems/System.h"
#include "Tiled.h"
#include "Util/Print.h"
#include "Util/Vector.h"

#define NULLOPT Corrade::Containers::NullOpt;

namespace whal {

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

Vector2i Scene::getStartPosition() {
    auto eStartLvlActive = getLoadedLevel(getStartLevel());
    if (!eStartLvlActive.isExpected()) {
        print("Got error in getStartPosition: ", eStartLvlActive.error());
        return {};
    }

    return eStartLvlActive.value()->spawnPoint;
}

Corrade::Containers::Optional<Level> Scene::getLevelAt(Vector2f worldPosTexels) const {
    for (Level lvl : allLevels) {
        if (worldPosTexels.x() >= lvl.worldPosOriginTexels.x() && worldPosTexels.x() < (lvl.worldPosOriginTexels.x() + lvl.sizeTexels.x()) &&
            worldPosTexels.y() < lvl.worldPosOriginTexels.y() && worldPosTexels.y() >= (lvl.worldPosOriginTexels.y() - lvl.sizeTexels.y())) {
            return lvl;
        }
    }
    return NULLOPT;
}

Expected<ActiveLevel*> Scene::getLoadedLevel(Level level) {
    for (auto& active : loadedLevels) {
        if (active == level) {
            return &active;
        }
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
    Vector2i worldOffsetPixels = Transform2D::texels(level.worldPosOriginTexels.x(), level.worldPosOriginTexels.y() - level.sizeTexels.y()).position;
    ActiveLevel lvl = {level, "", {}, worldOffsetPixels, {}, {}, {}};
    TileMap map = TileMap::parse(level.filepath.c_str(), lvl);
    print("loaded", map.name);

    std::vector<std::vector<s32>> collisionGrid;
    for (s32 x = 0; x < map.widthTiles; x++) {
        // std::vector<s32> collisionColumn;
        for (s32 y = 0; y < map.heightTiles; y++) {
            Transform2D trans = Transform2D(Transform2D::tiles(x, map.heightTiles - y).position + worldOffsetPixels);
            s32 ix = map.widthTiles * y + x;

            for (auto& layer : map.layers) {
                s32 blockID = layer.data.get()[ix];

                if (blockID != 0) {
                    // for now, am assuming everything in the base layer has collision
                    // collisionColumn.push_back(1);

                    Expected<Frame> frame = getTileFrame(map, blockID);
                    if (!frame.isExpected()) {
                        print(frame.error());
                        auto eEntity = createBlock(trans);
                        if (eEntity.isExpected()) {
                            lvl.childEntities.insert(eEntity.value());
                        }
                    } else {
                        Sprite sprite = Sprite(layer.metadata.depth, frame.value());

                        if (layer.metadata.depth == Depth::Level || layer.metadata.depth == Depth::Player) {
                            // not using collision mesh because i lose material info
                            const TileSet* tset = getTileSet(map, blockID);
                            WorldMaterial material = tset->materials[blockID - tset->firstgid];
                            auto eEntity = createBlock(trans, sprite, material);
                            if (eEntity.isExpected()) {
                                lvl.childEntities.insert(eEntity.value());
                            }

                        } else {
                            auto eEntity = createDecal(trans, sprite);
                            if (eEntity.isExpected()) {
                                lvl.childEntities.insert(eEntity.value());
                            }
                        }
                    }
                } else {
                    // collisionColumn.push_back(0);
                }
            }
        }
        // collisionGrid.push_back(collisionColumn);
    }

    // std::vector<SolidCollider> mesh;
    // makeCollisionMesh(collisionGrid, lvl);

    Game::instance().getScene().loadedLevels.push_back(lvl);

    return NULLOPT;
}

void unloadAndRemoveLevel(ActiveLevel& level) {
    // remove from Scene's list of loaded levels first,
    // so the EntityKilled listener doesn't mutate the list we're iterating
    // also copy it so erasing it doesn't invalidate our pointer

    ActiveLevel copy = level;
    Scene& scene = Game::instance().getScene();
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
        entity.kill();
    }
    print("unloaded level:", level.name);
}

void removeEntityFromLevel(ecs::Entity entity) {
    Scene& scene = Game::instance().getScene();
    if (scene.childEntities.erase(entity)) {
        if (entity.has<Name>()) {
            print("erasing entity", entity.get<Name>(), "from child lists");
        }
        return;
    }
    for (auto& lvl : scene.loadedLevels) {
        if (lvl.childEntities.erase(entity)) {
            if (entity.has<Name>()) {
                print("erasing entity", entity.get<Name>(), "from child lists");
            }
            break;
        }
    }
}

void addCollider(ActiveLevel& lvl, std::pair<s32, s32> startPoint, std::pair<s32, s32> endPoint) {
    s32 meshWidthTiles = endPoint.first - startPoint.first + 1;
    s32 meshHeightTiles = endPoint.second - startPoint.second + 1;

    s32 centerX = lvl.worldPosOriginTexels.x() * PIXELS_PER_TEXEL + startPoint.first * PIXELS_PER_TILE + (meshWidthTiles - 1) * PIXELS_PER_TILE / 2;
    s32 centerY = lvl.worldPosOriginTexels.y() * PIXELS_PER_TEXEL - startPoint.second * PIXELS_PER_TILE - (meshHeightTiles - 2) * PIXELS_PER_TILE / 2;

    Vector2i halflen = {meshWidthTiles * PIXELS_PER_TILE / 2, meshHeightTiles * PIXELS_PER_TILE / 2};
    auto collider = Collider::Solid(Transform2D({centerX, centerY}), halflen);

    auto eEntity = System::world->entity();
    if (!eEntity.isExpected()) {
        print("Error creating entity for mesh");
    } else {
        auto entity = eEntity.value();
        entity.add(collider);
        entity.add(Transform2D(collider.getShape().getPositionEdge(Vector2i::unitDown)));
        lvl.childEntities.insert(entity);
    }
}

void makeCollisionMesh(std::vector<std::vector<s32>>& collisionGrid, ActiveLevel& lvl) {
    // could be a LOT faster with std::vector<bool> + bitwise ops
    // timed @ 0.004 seconds for 1 level (quarter frame; can be asynch?)

    using Point = std::pair<s32, s32>;
    std::set<Point> visited;

    bool isStarted = false;
    Point startPoint;
    Point endPoint;

    for (size_t x = 0; x < collisionGrid.size(); x++) {
        for (size_t y = 0; y < collisionGrid[0].size(); y++) {
            Point point = {x, y};
            if (visited.find(point) != visited.end()) {
                continue;
            }
            visited.insert(point);

            bool isCollisionTile = collisionGrid[x][y] > 0;
            if (isCollisionTile) {
                if (!isStarted) {
                    isStarted = true;
                    startPoint = point;
                }
                endPoint = point;
            }
            if (!isCollisionTile || y == (collisionGrid[0].size() - 1) || (x == (collisionGrid.size() - 1))) {
                if (!isStarted) {
                    continue;
                }
                // fix endpoint's x coord, try expanding vertically
                bool isDone = false;
                bool endedEarly = false;
                Point newPoint;

                for (size_t xNew = endPoint.first + 1; xNew < collisionGrid.size(); xNew++) {
                    std::vector<Point> toInsert;
                    for (size_t yNew = startPoint.second; yNew <= static_cast<size_t>(endPoint.second); yNew++) {
                        newPoint = {xNew, yNew};  // pretty sure visit check is unecessary
                        toInsert.push_back(newPoint);
                        if (!collisionGrid[xNew][yNew]) {
                            endedEarly = true;
                            break;
                        }
                        isDone = (xNew == collisionGrid.size() - 1);
                    }
                    if (isDone || endedEarly) {
                        if (isDone) {
                            endPoint = newPoint;
                            for (Point p : toInsert) {
                                visited.insert(p);
                            }
                        }
                        break;
                    } else {
                        endPoint = newPoint;
                        for (Point p : toInsert) {
                            visited.insert(p);
                        }
                    }
                }
                addCollider(lvl, startPoint, endPoint);
                isStarted = false;
            }
        }
    }
}

}  // namespace whal
