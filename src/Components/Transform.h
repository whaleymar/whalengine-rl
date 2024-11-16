#pragma once

#include "Gfx/Depth.h"
#include "Util/Vector.h"

namespace whal {

enum class Direction : u8 { Neutral, N, S, E, W, NE, SE, NW, SW };

inline bool isCardinal(Direction d) {
    switch (d) {
    case Direction::N:
    case Direction::S:
    case Direction::E:
    case Direction::W:
        return true;
    default:
        return false;
    }
}

template <typename T>
inline Vector2T<T> directionToVector(Direction direction) {
    switch (direction) {
    case Direction::Neutral:
        return Vector2T<T>::ZERO;
    case Direction::N:
        return Vector2T<T>::UP;
    case Direction::S:
        return Vector2T<T>::DOWN;
    case Direction::E:
        return Vector2T<T>::RIGHT;
    case Direction::W:
        return Vector2T<T>::LEFT;
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

enum class Facing : u8 {
    Left = 0,
    Right = 1,
};

// entity position in pixels
struct Transform2D {
    Vector2i position;
    f32 rotationDegrees = 0.0;      // counterclockwise
    Facing facing = Facing::Right;  // draw calls flipped if facing left
    bool isManuallyMoved = true;    // if true, updates collider position without calling Collider.move
    Depth depth = Depth::Level;
    Vector2i pivotOffset = Vector2i::ZERO;  // used for rotation
    Vector2f scale = Vector2f::ONE;

    static Transform2D pixels(s32 x, s32 y);
    static Transform2D tiles(s32 x, s32 y);
    Vector2i getRotatedPosition() const;

    // Get an offset's transformed position
    Vector2i apply(Vector2i relOffset) const;

    // Calculate this Transform2D's root position using an offset's transformed position
    Vector2i applyInverse(Vector2i transformedPosition, Vector2i relOffset) const;

#ifndef NDEBUG
    void draw() const;
#endif
};

// TODO thinking of making this an OPTIONAL REPLACEMENT for Transform2D (an entity would have one or the other), but using floats for position
// and hopefully using CRTP to give this and Transform2D a common interface which works with the ECS
// and then defining some ecs::Any<Transform2D, PreciseTransform2D> thingy which systems can use
struct PrecisePosition {
    Vector2f position;

    static PrecisePosition fromTrans(Transform2D trans) { return PrecisePosition{trans.position.as<f32>()}; }
};

struct PreciseTransform2D {
    Vector2f position;

    f32 rotationDegrees = 0.0;      // counterclockwise
    Facing facing = Facing::Right;  // draw calls flipped if facing left
    bool isManuallyMoved = true;    // if true, updates collider position without calling Collider.move
    Depth depth = Depth::Level;
    Vector2i pivotOffset = Vector2i::ZERO;  // used for rotation
    Vector2f scale = Vector2f::ONE;
    f32 floatHeight = 0.0f;

    static PreciseTransform2D pixels(s32 x, s32 y);
    static PreciseTransform2D tiles(s32 x, s32 y);
    static PreciseTransform2D fromTrans(Transform2D trans) {
        return PreciseTransform2D{.position = trans.position.as<f32>(),
                                  .rotationDegrees = trans.rotationDegrees,
                                  .facing = trans.facing,
                                  .isManuallyMoved = trans.isManuallyMoved,
                                  .depth = trans.depth,
                                  .pivotOffset = trans.pivotOffset,
                                  .scale = trans.scale};
    }
    Vector2f getRotatedPosition() const;

    // Get an offset's transformed position
    Vector2f apply(Vector2f relOffset) const;

    // Calculate this Transform2D's root position using an offset's transformed position
    Vector2f applyInverse(Vector2f transformedPosition, Vector2f relOffset) const;
};

}  // namespace whal
