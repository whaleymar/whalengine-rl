#include "Level.h"

#include "Settings.h"

#include "Components/Collision.h"
#include "Components/Draw.h"
#include "Components/Transform.h"
#include "Entities/Block.h"

#include "Game.h"
#include "Gfx/Texture.h"
#include "Sys/System.h"
#include "Tiled.h"
#include "Util/Print.h"
#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

#define NULLOPT Corrade::Containers::NullOpt;

namespace whal {

Color getLightColor(LevelLighting lightLevel) {
    switch (lightLevel) {
    case LevelLighting::Normal:
        return WHITE;
    case LevelLighting::Dark:
        return BLACK;
    case LevelLighting::Dim:
        return Color(125, 125, 125, 255);
    }
}

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
    if (eActiveLevel.isExpected()) {
        initialSpawnPoint = eActiveLevel.value()->initialSpawnPoint;
    }
    return eActiveLevel;
}

Vector2i Scene::getClosestSpawnPoint(ActiveLevel& activeLevel, Vector2i referencePoint) {
    if (activeLevel.spawnPoints.size() == 0) {
        return activeLevel.worldPosOriginTexels.as<s32>();
    }

    Vector2i closestPoint = activeLevel.spawnPoints[0];
    f32 bestDistance = (closestPoint - referencePoint).as<f32>().len();

    for (size_t i = 1; i < activeLevel.spawnPoints.size(); i++) {
        Vector2i point = activeLevel.spawnPoints[i];
        f32 distance = (point - referencePoint).as<f32>().len();
        if (distance < bestDistance) {
            closestPoint = point;
            bestDistance = distance;
        }
    }

    return closestPoint;
}

Corrade::Containers::Optional<Level> Scene::getLevelAt(Vector2i worldPos) const {
    Vector2f worldPosTexels = worldPos.as<f32>() * FTEXELS_PER_PIXEL;
    for (Level lvl : allLevels) {
        if (worldPosTexels.x >= lvl.worldPosOriginTexels.x && worldPosTexels.x < (lvl.worldPosOriginTexels.x + lvl.sizeTexels.x) &&
            worldPosTexels.y < lvl.worldPosOriginTexels.y && worldPosTexels.y >= (lvl.worldPosOriginTexels.y - lvl.sizeTexels.y)) {
            return lvl;
        }
    }
    return NULLOPT;
}

Vector2i Scene::getClosestPositionInBounds(Vector2i worldPos) const {
    Vector2i worldPosTexels = (worldPos.as<f32>() * FTEXELS_PER_PIXEL).round();
    s32 minDistance = 999999;
    Vector2i closestPosition;
    for (Level lvl : allLevels) {
        const AABB lvlBox((lvl.worldPosOriginTexels + lvl.sizeTexels * Vector2f(0.5, -0.5)).as<s32>(), (lvl.sizeTexels * 0.5).as<s32>());

        const auto delta = worldPosTexels - lvlBox.getPosition();
        const auto half = lvlBox.getHalf();
        const auto closestPoint = lvlBox.getPosition() + Vector2i(clamp(delta.x, -half.x, half.x), clamp(delta.y, -half.y, half.y));

        s32 distance = (closestPoint - worldPosTexels).len();
        if (distance < minDistance) {
            minDistance = distance;
            closestPosition = closestPoint;
        }
    }

    return closestPosition * PIXELS_PER_TEXEL;
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

struct Tile {
    s32 gid;
    bool isFlipH;
    bool isFlipY;
    bool isRotate;
};

Tile getTile(u32 tileMask) {
    Tile tile;
    tile.isFlipH = tileMask & 0x80000000;   // Check if the 32nd bit is on
    tile.isFlipY = tileMask & 0x40000000;   // Check if the 31st bit is on
    tile.isRotate = tileMask & 0x20000000;  // Check if the 30th bit is on
    tile.gid = tileMask & 0x0FFFFFFF;       // Mask out the upper 4 bits to get the ID
    return tile;
}

Corrade::Containers::Optional<Error> loadLevel(const Level level) {
    Vector2i worldOffsetPixels = Transform2D::texels(level.worldPosOriginTexels.x, level.worldPosOriginTexels.y - level.sizeTexels.y).position;
    ActiveLevel lvl = {level, {}, {}, worldOffsetPixels, {}, {}, {}, {}};
    TileMap map = TileMap::parse(level.filepath.c_str(), lvl);
    print("loaded map: ", level.filepath);

    // std::vector<std::vector<s32>> collisionGrid;
    for (s32 x = 0; x < map.widthTiles; x++) {
        // std::vector<s32> collisionColumn;
        for (s32 y = 0; y < map.heightTiles; y++) {
            Transform2D trans = Transform2D(Transform2D::tiles(x, map.heightTiles - y).position + worldOffsetPixels);
            trans.facing = Facing::Right;
            s32 ix = map.widthTiles * y + x;

            for (auto& layer : map.layers) {
                u32 tileMask = layer.data[ix];
                Tile tile = getTile(tileMask);
                Tile originalTile = tile;
                s32 blockID = tile.gid;

                // the rotate flag technically means diagonal flipping or something idk it's some jank
                if (tile.isRotate) {
                    if (tile.isFlipY) {
                        tile.isFlipH = !tile.isFlipH;
                    }
                    if (!tile.isFlipH) {
                        tile.isFlipY = !tile.isFlipY;
                    } else if (!tile.isFlipY) {
                        tile.isFlipH = false;
                    }
                }

                if (originalTile.isRotate && originalTile.isFlipY && originalTile.isFlipH) {
                    trans.facing = Facing::Left;
                    // trans.rotationDegrees = 180;
                } else if (tile.isFlipH && !tile.isFlipY) {
                    trans.facing = Facing::Left;
                } else if (tile.isFlipY && !tile.isFlipH) {
                    trans.rotationDegrees = 180;
                    trans.facing = Facing::Left;
                } else if (tile.isFlipH && tile.isFlipY) {
                    trans.rotationDegrees = 180;
                }

                if (tile.isRotate) {
                    trans.rotationDegrees += 90;
                }

                if (blockID != 0) {
                    Expected<Frame> frame = getTileFrame(map, blockID);
                    if (!frame.isExpected()) {
                        print(frame.error());
                        auto eEntity = createBlock(trans);
                        if (eEntity.isExpected()) {
                            lvl.childEntities.insert(eEntity.value());
                        }
                    } else {
                        Sprite sprite = Sprite(layer.metadata.depth, frame.value());
                        sprite.isRotateAboutCenter = true;

                        // everything with Level/Player depth has collision
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
    print("unloaded level:", level.filepath);
}

void addCollider(ActiveLevel& lvl, std::pair<s32, s32> startPoint, std::pair<s32, s32> endPoint) {
    s32 meshWidthTiles = endPoint.first - startPoint.first + 1;
    s32 meshHeightTiles = endPoint.second - startPoint.second + 1;

    s32 centerX = lvl.worldPosOriginTexels.x * PIXELS_PER_TEXEL + startPoint.first * PIXELS_PER_TILE + (meshWidthTiles - 1) * PIXELS_PER_TILE / 2;
    s32 centerY = lvl.worldPosOriginTexels.y * PIXELS_PER_TEXEL - startPoint.second * PIXELS_PER_TILE - (meshHeightTiles - 2) * PIXELS_PER_TILE / 2;

    Vector2i halflen = {meshWidthTiles * PIXELS_PER_TILE / 2, meshHeightTiles * PIXELS_PER_TILE / 2};
    auto collider = Collider::Solid(Transform2D({centerX, centerY}), halflen);

    auto eEntity = System::world.entity();
    if (!eEntity.isExpected()) {
        print("Error creating entity for mesh");
    } else {
        auto entity = eEntity.value();
        entity.add(collider);
        entity.add(Transform2D(collider.getShape().getPositionEdge(Vector2i::unitDown)));
        lvl.childEntities.insert(entity);
    }
}

void makeCollisionMesh(const std::vector<std::vector<s32>>& collisionGrid, ActiveLevel& lvl) {
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
