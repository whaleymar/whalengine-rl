#pragma once

#include "Util/Vector.h"

namespace whal {

namespace ecs {

using EntityID = u32;
class Entity;

}  // namespace ecs

// in general, dead zone should be bigger than lookahead
struct Follow {
    Follow() = default;
    Follow(ecs::Entity target);

    void initTarget(ecs::Entity self);
    ecs::Entity getTarget() const;

    // ecs::Entity targetEntity;
    ecs::EntityID targetEntityID;

    Vector2i currentTarget;            // actual position we want to be at
    Vector2i lookAhead = {48, 16};     // offset from targetEntity that we aim for
    Vector2i deadZone = {96, 128};     // distance from target required to move in that direction
    Vector2i boundsX = {-5000, 5000};  // min and max X position we can be at
    Vector2i boundsY = {-5000, 5000};  // min and max Y position we can be at
    Vector2f damping = {1.0, 1.0};     // damping factor
    bool isMovingX = false;
    bool isMovingY = false;
    bool isTargetInitialized = false;

#ifndef NDEBUG
    ecs::EntityID debugTargetTrackerID;
    ecs::EntityID debugPositionTrackerID;
#endif  // !NDEBUG
};

}  // namespace whal
