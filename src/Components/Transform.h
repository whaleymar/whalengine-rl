#pragma once

#include "Gfx/Depth.h"
#include "Util/Vector.h"

namespace whal {

namespace ecs {
class Entity;
}

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
inline Vector2<T> directionToVector(Direction direction) {
    switch (direction) {
    case Direction::Neutral:
        return Vector2<T>::ZERO;
    case Direction::N:
        return Vector2<T>::UP;
    case Direction::S:
        return Vector2<T>::DOWN;
    case Direction::E:
        return Vector2<T>::RIGHT;
    case Direction::W:
        return Vector2<T>::LEFT;
    case Direction::NE:
        return Vector2<T>{1, 1};
    case Direction::SE:
        return Vector2<T>{1, -1};
    case Direction::NW:
        return Vector2<T>{-1, 1};
    case Direction::SW:
        return Vector2<T>{-1, -1};
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

struct Transform {
    // global transform (read only):
    Vector2f position;
    Vector2i positionPx;  // rounded replica of position, for convenience
    Vector2f scale = Vector2f::ONE;
    f32 rotation = 0.0f;     // degrees
    f32 floatHeight = 0.0f;  // RESEARCH make position Vector3f?

    // local transform (read/write):
    Vector2f localPosition = Vector2f::ZERO;
    Vector2f localScale = Vector2f::ONE;
    f32 localRotation = 0.0;
    Facing facing = Facing::Right;  // draw calls flipped if facing left
    bool isManuallyMoved = true;    // if true, updates collider position without calling Collider.move
    Depth depth = Depth::Level;
    Vector2f pivotOffset = Vector2f::ZERO;  // used for rotation

    static Transform world(s32 x, s32 y);
    static Transform world(Vector2i pos);
    static Transform tiles(s32 x, s32 y);
    static Transform tiles(Vector2i pos);

    // updates the local/global position:
    void translate(Vector2f moveAmount, ecs::Entity self);
    void rotate(f32 degrees, ecs::Entity self);
    void scaleBy(Vector2f amount, ecs::Entity self);

    // called when parent transforms are updated
    void setParent(const Transform& parentTrans, ecs::Entity self);
    void setParentPosition(Vector2f parentPosition, ecs::Entity self);
    void setParentScale(Vector2f parentScale, ecs::Entity self);
    void setParentRotation(f32 parentDegrees, ecs::Entity self);

    // these work in reverse, computing the local transform needed to get the desired global state
    void set(const Transform& trans, ecs::Entity self);
    void setPosition(Vector2f globalPosition, ecs::Entity self);
    void setScale(Vector2f globalScale, ecs::Entity self);
    void setRotation(f32 globalRotation, ecs::Entity self);

    Vector2f getRotatedPosition() const;

    // Get an offset's transformed position
    Vector2f apply(Vector2f relOffset) const;
    Vector2i apply(Vector2i relOffset) const;

    // Calculate this Transform's root position using an offset's transformed position
    Vector2f applyInverse(Vector2f transformedPosition, Vector2f relOffset) const;
    Vector2i applyInverse(Vector2i transformedPosition, Vector2i relOffset) const;

#ifndef NDEBUG
    void draw() const;
#endif
};

}  // namespace whal
