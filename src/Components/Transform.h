#pragma once

#include "Util/Vector.h"

namespace whal {

enum class Direction : u8 { Neutral, N, S, E, W, NE, SE, NW, SW };

template <typename T>
inline Vector2T<T> directionToVector(Direction direction) {
    switch (direction) {
    case Direction::Neutral:
        return Vector2T<T>::zero;
    case Direction::N:
        return Vector2T<T>::unitUp;
    case Direction::S:
        return Vector2T<T>::unitDown;
    case Direction::E:
        return Vector2T<T>::unitRight;
    case Direction::W:
        return Vector2T<T>::unitLeft;
    case Direction::NE:
        return Vector2T<T>{1, 1};
    case Direction::SE:
        return Vector2T<T>{1, -1};
    case Direction::NW:
        return Vector2T<T>{-1, 1};
    case Direction::SW:
        return Vector2T<T>{-1, -1};
    }
}

inline f32 directionToAngle(Direction direction) {
    switch (direction) {
    case Direction::Neutral:
        return 0.0f;
    case Direction::N:
        return 90.0f;
    case Direction::S:
        return 270.0f;
    case Direction::E:
        return 0.0f;
    case Direction::W:
        return 180.0f;
    case Direction::NE:
        return 45.0f;
    case Direction::SE:
        return 315.0f;
    case Direction::NW:
        return 135.0f;
    case Direction::SW:
        return 225.0f;
    }
}

enum Facing : u8 {
    Left = 0,
    Right = 1,
};

// entity position in pixels
struct Transform2D {
    Vector2i position;
    f32 rotationDegrees = 0.0;      // counterclockwise
    Facing facing = Facing::Right;  // draw calls flipped if facing right
    bool isManuallyMoved = true;    // tells physics system to sync collider position

    static Transform2D texels(s32 x, s32 y);
    static Transform2D tiles(s32 x, s32 y);
};

struct PrecisePosition {
    Vector2f position;

    static PrecisePosition fromTrans(Transform2D trans) { return PrecisePosition{toFloatVec(trans.position)}; }
};

}  // namespace whal
