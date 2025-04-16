#include "Path.h"

#include <algorithm>
#include <queue>

#include "Map/Scene.h"
#include "Map/Tiled.h"

namespace whal {

template <typename T>
class Matrix2D {
public:
    Matrix2D(s32 width, s32 height, T zeroVal) : mWidth(width), mHeight(height), mZeroVal(zeroVal) {
        mData = std::vector<T>(mWidth * mHeight, mZeroVal);
    }

    T get(s32 x, s32 y) const {
        assert(x >= 0 && x < mWidth && "X is out of bounds");
        assert(y >= 0 && y < mHeight && "Y is out of bounds");
        return mData[mWidth * y + x];
    }
    void set(s32 x, s32 y, T val) {
        assert(x >= 0 && x < mWidth && "X is out of bounds");
        assert(y >= 0 && y < mHeight && "Y is out of bounds");
        mData[mWidth * y + x] = val;
    }
    bool isEmpty(s32 x, s32 y) const { return get(x, y) == mZeroVal; }

    bool isInBounds(s32 x, s32 y) const { return x >= 0 && x < mWidth && y >= 0 && y < mHeight; }

private:
    s32 mWidth;
    s32 mHeight;
    T mZeroVal;
    std::vector<T> mData;
};

static inline f32 distance(const Vector2i& v1, const Vector2i& v2) {
    return (v1 - v2).as<f32>().len();
}

static inline bool isValidTile(const Vector2i& v1, const std::vector<std::vector<u8>>& navGridStatic,
                               const std::vector<std::vector<u32>>& navGridDynamic, u32 entityID, s32 height) {
    for (s32 i = 0; i < height; ++i) {
        const s32 yIx = v1.y - i;
        if (yIx < 0) {
            return false;
        }
        if (!navGridStatic[v1.x][yIx]) {
            return false;
        }
        u32 id = navGridDynamic[v1.x][yIx];
        if (id > 0 && id != entityID) {
            return false;
        }
    }
    return true;
}

// checks if destination tile is valid, as well as the tiles in each cardinal direction of travel
static inline bool isValidTileDiagonal(const Vector2i& v1, const std::vector<std::vector<u8>>& navGridStatic,
                                       const std::vector<std::vector<u32>>& navGridDynamic, u32 entityID, s32 height, Vector2i moveDir) {
    for (s32 i = 0; i < height; ++i) {
        const s32 yIx = v1.y - i;
        if (yIx < 0) {
            return false;
        }
        if (!navGridStatic[v1.x][yIx]) {
            return false;
        }
        if (!navGridStatic[v1.x][yIx - moveDir.y]) {
            return false;
        }
        if (!navGridStatic[v1.x - moveDir.x][yIx]) {
            return false;
        }
        u32 id = navGridDynamic[v1.x][yIx];
        if (id > 0 && id != entityID) {
            return false;
        }
        id = navGridDynamic[v1.x][yIx - moveDir.y];
        if (id > 0 && id != entityID) {
            return false;
        }
        id = navGridDynamic[v1.x - moveDir.x][yIx];
        if (id > 0 && id != entityID) {
            return false;
        }
    }
    return true;
}

Path findPath(u32 entityID, const Vector2i startWorldPosition, const Vector2i targetWorldPosition, const TileMap& level, s32 height) {
    using namespace std;

    static const Vector2i directions[] = {Vector2i::RIGHT, Vector2i::LEFT,  Vector2i::UP,    Vector2i::DOWN,
                                          Vector2i(1, 1),  Vector2i(1, -1), Vector2i(-1, 1), Vector2i(-1, -1)};
    static const f32 costs[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.41f, 1.41f, 1.41f, 1.41f};
    constexpr size_t nDirections = 8;

    // Convert world positions into level tile positions.
    // Level origin is top left, so negate Y values.
    // Clamp values to lvl in case of rounding issues.
    const Vector2i start = level.worldPositionToTileClamped(startWorldPosition);
    const Vector2i target = level.worldPositionToTileClamped(targetWorldPosition);

    const auto comp = [](const pair<f32, Vector2i>& elem1, const pair<f32, Vector2i>& elem2) { return elem1.first > elem2.first; };
    priority_queue<pair<f32, Vector2i>, vector<pair<f32, Vector2i>>, decltype(comp)> openSet(comp);

    Matrix2D<Vector2i> cameFrom = Matrix2D<Vector2i>(level.sizeTiles.x, level.sizeTiles.y, Vector2i(-1, -1));
    Matrix2D<f32> gScore = Matrix2D<f32>(level.sizeTiles.x, level.sizeTiles.y, -1.0f);

    openSet.emplace(0.0f, start);
    gScore.set(start.x, start.y, 0.0f);

    Vector2i closest = start;
    f32 closestDistance = distance(start, target);

    while (!openSet.empty()) {
        const Vector2i current = openSet.top().second;
        openSet.pop();

        if (current == target) {
            break;
        }

        const f32 currentDistance = distance(current, target);
        if (currentDistance < closestDistance) {
            closest = current;
            closestDistance = currentDistance;
        }

        // Explore neighbors
        for (size_t i = 0; i < nDirections; i++) {
            const Vector2i neighbor = current + directions[i];
            if (i >= 4) {
                if (neighbor.x < 0 || neighbor.x >= level.sizeTiles.x || neighbor.y < 0 || neighbor.y >= level.sizeTiles.y ||
                    !isValidTileDiagonal(neighbor, level.navGrid, level.navGridDynamic, entityID, height, directions[i])) {
                    continue;
                }

            } else {
                if (neighbor.x < 0 || neighbor.x >= level.sizeTiles.x || neighbor.y < 0 || neighbor.y >= level.sizeTiles.y ||
                    !isValidTile(neighbor, level.navGrid, level.navGridDynamic, entityID, height)) {
                    continue;
                }
            }

            const f32 newGScore = gScore.get(current.x, current.y) + costs[i];
            if (gScore.isEmpty(neighbor.x, neighbor.y) || newGScore < gScore.get(neighbor.x, neighbor.y)) {
                // update scores
                cameFrom.set(neighbor.x, neighbor.y, current);
                gScore.set(neighbor.x, neighbor.y, newGScore);
                f32 fScore = newGScore + distance(neighbor, target);
                openSet.emplace(fScore, neighbor);
            }
        }
    }

    // reconstruct path by backtracking from closest position (or target)
    Path path;
    path.start = startWorldPosition;
    path.target = targetWorldPosition;
    const Vector2i pathTarget = (gScore.isInBounds(target.x, target.y) && !gScore.isEmpty(target.x, target.y) ? target : closest);
    Vector2i step = pathTarget;
    while (true) {
        Vector2i from = cameFrom.get(step.x, step.y);
        Vector2i delta = (step - from) * Vector2i(1, -1);  // re-negate Y movement
        path.tiles.push_back(delta);
        if (step == start) {
            break;
        }
        step = from;
    }
    path.tiles.pop_back();  // don't need start position
    std::reverse(path.tiles.begin(), path.tiles.end());
    return path;
}

}  // namespace whal
