#pragma once

#include "Util/Vector.h"
#include "whalECS/src/ECS.h"

namespace whal {

struct Attach {
    Attach() = default;
    Attach(ecs::Entity target, Vector2i offset = {0, 0});

    void initTarget(ecs::Entity self);

    ecs::Entity targetEntity;
    Vector2i offsetTexels;
};

// in general, dead zone should be bigger than lookahead
struct Follow {
    Follow() = default;
    Follow(ecs::Entity target);

    void initTarget(ecs::Entity self);

    ecs::Entity targetEntity;

    Vector2i currentTarget;                  // actual position we want to be at
    Vector2i lookAheadTexels = {48, 16};     // offset from targetEntity that we aim for
    Vector2i deadZoneTexels = {96, 128};     // distance from target required to move in that direction
    Vector2i boundsXTexels = {-5000, 5000};  // min and max X position we can be at
    Vector2i boundsYTexels = {-5000, 5000};  // min and max Y position we can be at
    Vector2f damping = {1.0, 1.0};           // damping factor
    bool isMovingX = false;
    bool isMovingY = false;
    bool isTargetInitialized = false;

#ifndef NDEBUG
    ecs::Entity debugTargetTracker;
    ecs::Entity debugPositionTracker;
#endif  // !NDEBUG
};

// give system which deletes children in ondelete
struct Children {
    std::vector<ecs::Entity> entities;

    void add(ecs::Entity entity);
};

}  // namespace whal
