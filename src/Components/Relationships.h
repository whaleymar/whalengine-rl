#pragma once

#include <vector>
#include "Map/ComponentFactory.h"
#include "Util/Vector.h"

namespace whal {

namespace ecs {

using EntityID = u32;
class Entity;

}  // namespace ecs

struct Attach : ISerialize<Attach, ComponentFactory> {
    enum class DirectionParam { IgnoreFacing, UseFacingForOffset, UseFacingForAll };

    Attach() = default;
    Attach(ecs::Entity target, Vector2i offset = {0, 0}, DirectionParam directionParam_ = DirectionParam::IgnoreFacing);

    void initTarget(ecs::Entity self);
    ecs::Entity getTarget() const;

    ecs::EntityID targetEntityID;
    Vector2i offset;
    DirectionParam directionParam = DirectionParam::IgnoreFacing;

    // managed:
    bool isTargetInitialized = false;

    static void loadImpl(ecs::Entity entity, void* data);
};

struct Orbit : ISerialize<Orbit, ComponentFactory> {
    Orbit() = default;
    Orbit(ecs::Entity target, s32 radius_, f32 rotationsPerSecond_, Vector2i targetOffset_ = {0, 0});

    void initTarget(ecs::Entity self);
    ecs::Entity getTarget() const;

    ecs::EntityID targetID;
    s32 radius;
    f32 rotationsPerSecond;
    Vector2i targetOffset;
    Vector2i selfOffset;

    // managed:
    f32 currentAngle;
    bool isTargetInitialized = false;

    static void loadImpl(ecs::Entity entity, void* data);
};

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

// give system which deletes children in ondelete
struct Children {
    // std::vector<ecs::Entity> entities;
    std::vector<ecs::EntityID> entityIDs;

    void add(ecs::Entity entity);
};

}  // namespace whal
