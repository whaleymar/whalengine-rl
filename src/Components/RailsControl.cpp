#include "RailsControl.h"

#include "Components/Transform.h"
#include "Map/Tiled.h"
#include "Map/TiledParse.h"
#include "Settings.h"
#include "Util/Print.h"
#include "json.hpp"

namespace whal {

constexpr f32 SPEED_CURVE_EPSILON = 0.01;

RailsControl::RailsControl(f32 moveSpeed_, std::vector<CheckPoint> checkPoints_, f32 waitTime_, CycleBehavior cycleBehavior_,
                           ArrivalCallback callback)
    : mCheckpoints(checkPoints_), speed(moveSpeed_), waitTime(waitTime_), arrivalCallback(callback), curActionTime(waitTime_),
      endBehavior(cycleBehavior_) {}

void RailsControl::setCheckpoints(std::vector<CheckPoint>& checkpoints, Transform& trans, ecs::Entity e) {
    mCheckpoints = std::move(checkpoints);
    prepareForFirstStep(trans, e);
}

RailsControl::CheckPoint RailsControl::getTarget() const {
    return mCheckpoints[curTarget];
}

void RailsControl::startManually() {
    isVelocityUpdateNeeded = true;
    curActionTime = waitTime;
}

void RailsControl::step() {
    startPosition = getTarget().position.as<f32>();

    if (isForward) {
        curTarget++;
        if (curTarget == mCheckpoints.size()) {
            if (isLooping()) {
                curTarget = 0;
            } else {
                curTarget -= 2;
                isForward = false;
            }
        }
    } else {
        if (curTarget == 0) {
            curTarget = 1;
            isForward = true;
        } else {
            curTarget--;
        }
    }
    curActionTime = 0;
}

f32 RailsControl::getSpeed(Vector2i currentPosition) {
    // this is super hacky
    // because my easeIn/easeOut functions describe 2nd order functions
    // but i need to set a velocity, not position, so the physics system will work
    // So I calculate the position I should be in, according to the movement function.
    // Then I use the current position to get the velocity I should go to get there by the next frame.
    //
    // I feel like I should be using deltatime but it breaks things
    // but idk it Just Works

    const Vector2f targetPosf = getTarget().position.as<f32>();

    const f32 segmentDistance = (targetPosf - startPosition).len();
    const f32 expectedSegmentTime = segmentDistance / speed;
    f32 progress = ease(0.0f, 1.0f, curActionTime / expectedSegmentTime, getTarget().movement);

    if ((1 - progress) < SPEED_CURVE_EPSILON) {
        isVelocityUpdateNeeded = false;
    }
    const Vector2f newPos = startPosition.lerp(targetPosf, progress);
    return (newPos - currentPosition.as<f32>()).len() * PIXELS_PER_TILE;  // idfk why this works
}

bool RailsControl::isValid() const {
    return mCheckpoints.size() > 1;
}

bool RailsControl::isLooping() const {
    return endBehavior == CycleBehavior::MANUAL_ALLSTEPS_LOOP || endBehavior == CycleBehavior::MANUAL_FIRSTSTEP_LOOP ||
           endBehavior == CycleBehavior::AUTOMATIC_LOOP;
}

bool RailsControl::isNextStepAutomatic() const {
    return endBehavior == CycleBehavior::AUTOMATIC_LOOP || endBehavior == CycleBehavior::AUTOMATIC_BACKTRACK ||
           (curTarget != 0 && (endBehavior == CycleBehavior::MANUAL_FIRSTSTEP_LOOP || endBehavior == CycleBehavior::MANUAL_FIRSTSTEP_BACKTRACK));
}

void RailsControl::prepareForFirstStep(Transform& trans, ecs::Entity e) {
    if (isValid()) {
        // set transform to match starting checkpoint
        Vector2i target = getTarget().position;
        startPosition = target.as<f32>();
        trans.setPosition(target.as<f32>(), e);
    } else {
        // o.w., make sure start position matches transform
        startPosition = trans.position;
    }
}

// returns true if checkpoints form a cycle
static bool loadCheckpoints(const nlohmann::json& checkpointData, std::vector<RailsControl::CheckPoint>& dstCheckpoints, const LoadContext& ctx) {
    static const char* KEY_VALUE = "value";

    // generic rewrite:
    const s32 parentX = readInt(checkpointData, "x");
    const s32 parentY = readInt(checkpointData, "y");
    assert(checkpointData.contains("properties") && "Checkpoint object has no properties");
    const auto& properties = checkpointData["properties"];
    std::vector<Ease> moveProps;
    for (const auto& moveProperty : properties) {
        const s32 moveIx = moveProperty[KEY_VALUE];
        moveProps.push_back(static_cast<Ease>(moveIx));
    }

    bool isCycle = checkpointData.contains("polygon");
    std::string pathKey;
    if (isCycle) {
        pathKey = "polygon";
    } else {
        pathKey = "polyline";
    }
    size_t ix = 0;
    for (const auto& point : checkpointData[pathKey]) {
        const s32 x = readInt(point, "x");
        const s32 y = readInt(point, "y");
        const Vector2i mapPos = {x + parentX, parentY + y};
        const Vector2i trans = getMapTransform(mapPos, Vector2i::ZERO, ctx.parent).positionPx;

        Ease moveType;
        if (ix >= moveProps.size()) {
            print("Checkpoints object with ID", readInt(checkpointData, "id"), "has", moveProps.size(), "move type params but it has more points");
            moveType = Ease::Linear;
        } else {
            moveType = moveProps[ix];
        }
        RailsControl::CheckPoint chkPoint(trans, moveType);
        dstCheckpoints.push_back(chkPoint);

        ix++;
    }

    return isCycle;
}

void RailsControl::loadImpl(ecs::Entity entity, const LoadContext& ctx) {
    std::vector<RailsControl::CheckPoint> checkpoints;
    bool isCycle = false;
    if (ctx.values->contains("Checkpoints")) {
        s32 id = (*ctx.values)["Checkpoints"];
        const nlohmann::json checkPointObj = ctx.allObjects.at(ctx.idToIndex.at(id).first);
        isCycle = loadCheckpoints(checkPointObj, checkpoints, ctx);
    }

    RailsControl rails = entity.has<RailsControl>() ? entity.get<RailsControl>() : RailsControl{};
    rails.setCheckpoints(checkpoints, entity.get<Transform>(), entity);

    std::string cycleBehavior = "ManualStart";
    tryRead(*ctx.values, "CycleBehavior", &cycleBehavior);
    if (isCycle) {
        if (cycleBehavior == "Automatic") {
            rails.endBehavior = RailsControl::CycleBehavior::AUTOMATIC_LOOP;
        } else if (cycleBehavior == "ManualStart") {
            rails.endBehavior = RailsControl::CycleBehavior::MANUAL_FIRSTSTEP_LOOP;
        } else {
            rails.endBehavior = RailsControl::CycleBehavior::MANUAL_ALLSTEPS_LOOP;
        }
    } else {
        if (cycleBehavior == "Automatic") {
            rails.endBehavior = RailsControl::CycleBehavior::AUTOMATIC_BACKTRACK;
        } else if (cycleBehavior == "ManualStart") {
            rails.endBehavior = RailsControl::CycleBehavior::MANUAL_FIRSTSTEP_BACKTRACK;
        } else {
            rails.endBehavior = RailsControl::CycleBehavior::MANUAL_ALLSTEPS_BACKTRACK;
        }
    }

    tryRead(*ctx.values, "speed", &rails.speed);
    tryRead(*ctx.values, "waitTime", &rails.waitTime);

    entity.add(rails);
}

}  // namespace whal
