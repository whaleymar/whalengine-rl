#include "Path.h"
#include <algorithm>
#include <queue>
#include <unordered_map>
#include "Gfx/Coordinates.h"
#include "Map/Level.h"
#include "Settings.h"

// RESEARCH
// Extensions I can think of:
// 1. A "size" parameter: findPath assumes your collider is 1x1 tile. If size is bigger should check adjacent tiles to make sure path is valid
// 2. knowing the total distance of the path could be useful

namespace whal {

static inline f32 distance(const Vector2i& v1, const Vector2i& v2) {
    return (v1.as<f32>() - v2.as<f32>()).len();
}

static inline bool isValidTile(const Vector2i& v1, const ActiveLevel& level, s32 height) {
    for (s32 i = 0; i < height; ++i) {
        if (!level.navGrid[v1.x][v1.y + i]) {
            return false;
        }
    }
    return true;
    // return level.navGrid[v1.x][v1.y];
}

Path findPath(const Vector2i startWorldPosition, const Vector2i targetWorldPosition, const ActiveLevel& level, s32 height) {
    using namespace std;

    static const Vector2i directions[] = {Vector2i::RIGHT, Vector2i::LEFT,  Vector2i::UP,    Vector2i::DOWN,
                                          Vector2i(1, 1),  Vector2i(1, -1), Vector2i(-1, 1), Vector2i(-1, -1)};
    static const f32 costs[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.41f, 1.41f, 1.41f, 1.41f};
    // constexpr size_t nDirections = 8;
    constexpr size_t nDirections = 4;  // TEMP

    // convert world positions into level tile positions
    // level origin is top left, so negate Y values
    const Vector2i start = (worldToTileCoords(startWorldPosition) - worldToTileCoords(level.position.as<s32>())) * Vector2i(1, -1);
    const Vector2i target = (worldToTileCoords(targetWorldPosition) - worldToTileCoords(level.position.as<s32>())) * Vector2i(1, -1);
    // print("converted start & target from ", startWorldPosition, targetWorldPosition, "to", start, target);

    const auto comp = [](const pair<f32, Vector2i>& elem1, const pair<f32, Vector2i>& elem2) { return elem1.first > elem2.first; };
    priority_queue<pair<f32, Vector2i>, vector<pair<f32, Vector2i>>, decltype(comp)> openSet(comp);
    unordered_map<Vector2i, Vector2i, Vector2iHash> cameFrom;
    unordered_map<Vector2i, f32, Vector2iHash> gScore;
    unordered_map<Vector2i, f32, Vector2iHash> fScore;

    openSet.emplace(0.0f, start);
    gScore[start] = 0.0f;
    fScore[start] = distance(start, target);

    Vector2i closest = start;
    f32 closestDistance = distance(start, target);
    const Vector2i lvlSize = level.size.as<s32>() / PIXELS_PER_TILE;

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
            if (neighbor.x < 0 || neighbor.x >= lvlSize.x || neighbor.y < 0 || neighbor.y >= lvlSize.y || !isValidTile(neighbor, level, height)) {
                continue;
            }

            const f32 newGScore = gScore[current] + costs[i];
            if (gScore.find(neighbor) == gScore.end() || newGScore < gScore[neighbor]) {
                // update scores
                cameFrom[neighbor] = current;
                gScore[neighbor] = newGScore;
                fScore[neighbor] = newGScore + distance(neighbor, target);
                openSet.emplace(fScore[neighbor], neighbor);
            }
        }
    }

    // reconstruct path by backtracking from closest position (or target)
    Path path;
    path.start = startWorldPosition;
    path.target = targetWorldPosition;
    const Vector2i pathTarget = (gScore.find(target) != gScore.end() ? target : closest);
    Vector2i step = pathTarget;
    while (true) {
        Vector2i from = cameFrom[step];
        Vector2i delta = (step - from) * Vector2i(1, -1);  // re-negate Y movement
        path.tiles.push_back(delta);
        if (step == start) {
            break;
        }
        step = from;
    }
    path.tiles.pop_back();
    std::reverse(path.tiles.begin(), path.tiles.end());
    return path;
}

}  // namespace whal
