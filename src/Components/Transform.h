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

inline Vector2i directionToVector(Direction direction) {
    switch (direction) {
    case Direction::Neutral:
        return Vector2i::ZERO;
    case Direction::N:
        return Vector2i::UP;
    case Direction::S:
        return Vector2i::DOWN;
    case Direction::E:
        return Vector2i::RIGHT;
    case Direction::W:
        return Vector2i::LEFT;
    case Direction::NE:
        return Vector2i{1, 1};
    case Direction::SE:
        return Vector2i{1, -1};
    case Direction::NW:
        return Vector2i{-1, 1};
    case Direction::SW:
        return Vector2i{-1, -1};
    }
}

inline Direction directionFromVector(Vector2i vec) {
    if (vec.x > 0) {
        if (vec.y > 0) {
            return Direction::NE;
        } else if (vec.y < 0) {
            return Direction::SE;
        } else {
            return Direction::E;
        }
    } else if (vec.x < 0) {
        if (vec.y > 0) {
            return Direction::NW;
        } else if (vec.y < 0) {
            return Direction::SW;
        } else {
            return Direction::W;
        }
    } else {
        if (vec.y > 0) {
            return Direction::N;
        } else if (vec.y < 0) {
            return Direction::S;
        } else {
            return Direction::Neutral;
        }
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

// Any method that takes an entity will update the transforms of that entity's children
struct Transform {
    friend class TransformBuilder;

    // global transform (use setters for these):
    Vector2f position;
    Vector2i positionPx;             // rounded replica of position, for convenience
    Vector2f scale = Vector2f::ONE;  // negative scale to flip sprites
    f32 rotation = 0.0f;             // degrees
    f32 z = 0.0f;

    // local transform (don't edit these):
    Vector2f _localPosition = Vector2f::ZERO;
    Vector2f _localScale = Vector2f::ONE;
    f32 _localRotation = 0.0f;
    f32 _localZ = 0.0f;

    bool isManuallyMoved = true;  // if true, updates collider position without calling Collider.move
    bool isDirty = true;          // true if entity has moved since the last frame was rendered (Renderer is in charge of clearing this)
    Depth depth = Depth::Level;
    Vector2f pivotOffset = Vector2f::ZERO;  // used for rotation // RESEARCH maybe can get rid of this by using a parent entity for the offset?

    static Transform world(s32 x, s32 y);
    static Transform world(Vector2i pos);
    static Transform world(Vector2f pos);
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
    void setParentZ(f32 parentZ, ecs::Entity self);

    // these work in reverse, computing the local transform needed to get the desired global state
    void set(const Transform& trans, ecs::Entity self);
    void setPosition(Vector2f globalPosition, ecs::Entity self);
    void setScale(Vector2f globalScale, ecs::Entity self);
    void setRotation(f32 globalRotation, ecs::Entity self);
    void setZ(f32 globalZ, ecs::Entity self);

    void setFacing(Facing dir, ecs::Entity self);

    Vector2f getRotatedPosition() const;
    Vector2i getRotatedPositionInt() const;

    // ignores the Z axis (floatHeight)
    Vector2f getRotatedPosition2D() const;
    // ignores the Z axis (floatHeight)
    Vector2i getRotatedPositionInt2D() const;

    // Get an offset's transformed position
    Vector2f apply(Vector2f relOffset) const;
    Vector2i apply(Vector2i relOffset) const;

    // ignores the Z axis (floatHeight)
    Vector2f apply2D(Vector2f relOffset) const;
    // ignores the Z axis (floatHeight)
    Vector2i apply2D(Vector2i relOffset) const;

    // Calculate this Transform's root position using an offset's transformed position
    Vector2f applyInverse(Vector2f transformedPosition, Vector2f relOffset) const;
    Vector2i applyInverse(Vector2i transformedPosition, Vector2i relOffset) const;

    // ignores the Z axis (floatHeight)
    Vector2f apply2DInverse(Vector2f transformedPosition, Vector2f relOffset) const;
    // ignores the Z axis (floatHeight)
    Vector2i apply2DInverse(Vector2i transformedPosition, Vector2i relOffset) const;

#ifndef NDEBUG
    void draw() const;
#endif
};

// This is for building Transforms that aren't attached to any entity
class TransformBuilder {
public:
    TransformBuilder() = default;
    TransformBuilder(const Transform&);
    TransformBuilder(ecs::Entity);
    TransformBuilder& translate(Vector2f moveAmount);
    TransformBuilder& scaleBy(Vector2f mult);
    TransformBuilder& rotate(f32 degrees);
    TransformBuilder& position(Vector2f globalPosition);
    TransformBuilder& scale(Vector2f globalScale);
    TransformBuilder& rotation(f32 globalRotation);
    TransformBuilder& z(f32 globalHeight);
    TransformBuilder& depth(Depth depth);
    TransformBuilder& facing(Facing facing);
    Transform build() const;

private:
    Transform mTrans;
};

}  // namespace whal
