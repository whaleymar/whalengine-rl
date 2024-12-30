#include "Relationships.h"

#include "Components/Transform.h"
#include "Map/Tiled.h"
#include "Util/Print.h"
#include "json.hpp"
#include "whalECS/src/ECS.h"

namespace whal {

void Attach::loadImpl(ecs::Entity entity, void* data) {
    const LoadContext& ctx = *static_cast<LoadContext*>(data);

    s32 targetId;
    if (!tryRead(ctx.values, "target", &targetId)) {
        print("Entity with Map id ", ctx.entityData.id, "has attach component with no target");
        return;
    }

    // tryReadVal(ctx.values, "DirectionParam", &attach.directionParam);

    ecs::Entity target = ctx.idToIndex.at(targetId).second;
    target.addChild(entity);

    const Vector2i thisPosition = entity.get<Transform>().positionPx;

    // other isn't guaranteed to have been parsed. Calculate its transform manually
    const auto& targetObj = ctx.allObjects[ctx.idToIndex.at(targetId).first];
    Vector2i otherDims = getObjectSize(targetObj);
    const Vector2i otherPosition = getMapTransform(readVector2i(targetObj), otherDims, ctx.parent, ctx.parentSize).positionPx;

    entity.get<Transform>().translate((thisPosition - otherPosition).as<f32>(), entity);
}

Orbit::Orbit(ecs::Entity target, s32 radius_, f32 rotationsPerSecond_, Vector2i targetOffset_)
    : targetID(target.id()), radius(radius_), rotationsPerSecond(rotationsPerSecond_), targetOffset(targetOffset_) {}

// adds self as child of target
void Orbit::initTarget(ecs::Entity self) {
    isTargetInitialized = true;
    ecs::Entity targetEntity(targetID);

    // initialize current angle
    const Vector2i delta = self.get<Transform>().positionPx - targetEntity.get<Transform>().positionPx;
    currentAngle = delta.isZero() ? 0.0f : delta.as<f32>().angle();
}

void Orbit::loadImpl(ecs::Entity entity, void* data) {
    const LoadContext& ctx = *static_cast<LoadContext*>(data);
    Orbit orbit = entity.has<Orbit>() ? entity.get<Orbit>() : Orbit{};

    tryRead(ctx.values, "RotationsPerSecond", &orbit.rotationsPerSecond);

    const Vector2i entityTrans = entity.get<Transform>().positionPx;

    if (!ctx.values.contains("Target")) {
        print("Error: Orbit component requires a Target");
        return;
    }

    s32 shapeId = readInt(ctx.values, "Target");
    const auto& shapeObj = ctx.allObjects[ctx.idToIndex.at(shapeId).first];
    Vector2i otherDimensions = Vector2i::ZERO;
    tryRead(shapeObj, "width", "height", &otherDimensions);
    const Vector2i otherTrans = getMapTransform(readVector2i(shapeObj), otherDimensions, ctx.parent, ctx.parentSize).positionPx;

    orbit.radius = std::round((entityTrans - otherTrans).as<f32>().len());
    orbit.targetID = ctx.idToIndex.at(shapeId).second.id();

    entity.add(orbit);
}

Follow::Follow(ecs::Entity target_) : targetEntityID(target_.id()) {}

void Follow::initTarget(ecs::Entity self) {
    isTargetInitialized = true;
    ecs::Entity targetEntity(targetEntityID);
    currentTarget = targetEntity.get<Transform>().positionPx;
}

}  // namespace whal
