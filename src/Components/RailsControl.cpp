#include "RailsControl.h"

#include "Components/Transform.h"
#include "Settings.h"
#include "Util/MathUtil.h"

namespace whal {

constexpr f32 SPEED_CURVE_EPSILON = 0.01;

RailsControl::RailsControl(f32 moveSpeed_, std::vector<CheckPoint> checkPoints_, f32 waitTime_, CycleBehavior cycleBehavior_,
                           ArrivalCallback callback)
    : mCheckpoints(checkPoints_), speed(moveSpeed_), waitTime(waitTime_), arrivalCallback(callback), curActionTime(waitTime_),
      endBehavior(cycleBehavior_) {}

void RailsControl::setCheckpoints(std::vector<CheckPoint>& checkpoints, Transform2D& trans) {
    mCheckpoints = std::move(checkpoints);
    prepareForFirstStep(trans);
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

// TODO consolidate with other easing enum
f32 RailsControl::getSpeed(Vector2i currentPosition) {
    // this is super hacky
    // because my easeIn/easeOut functions describe 2nd order functions
    // but i need to set a velocity, not position, so the physics system will work
    // So I calculate the position I should be in, according to the movement function.
    // Then I use the current position to get the velocity I should go to get there by the next frame.
    //
    // I feel like I should be using deltatime but it breaks things
    // but idk it Just Works

    // TODO use derivates of easein/out functions

    const Vector2f targetPosf = getTarget().position.as<f32>();

    const f32 segmentDistance = (targetPosf - startPosition).len();
    const f32 expectedSegmentTime = segmentDistance / (speed * FPIXELS_PER_TEXEL);  // speed is in texels/sec, but pos is in pixels

    f32 t = clamp(curActionTime / expectedSegmentTime, 0.0f, 1.0f);
    f32 progress;
    switch (getTarget().movement) {
    case Movement::LINEAR:
        progress = t;
        break;

    case Movement::EASEIO_SINE:
        progress = easeInOutSine(0, 1, t);
        break;

    case Movement::EASEIO_BEZIER:
        progress = easeInOutBezier(0, 1, t);
        break;

    case Movement::EASEI_QUAD:
        progress = easeInQuad(0, 1, t);
        break;

    case Movement::EASEI_CUBE:
        progress = easeInCubic(0, 1, t);
        break;

    case Movement::EASEO_QUAD:
        progress = easeOutQuad(0, 1, t);
        break;

    case Movement::EASEO_CUBE:
        progress = easeOutCubic(0, 1, t);
        break;
    }

    // print("segment distance:", segmentDistance, "speed", speed, "expected segment time:", expectedSegmentTime, "current action time",
    // curActionTime,
    //       "t", t, "progress", progress);

    if ((1 - progress) < SPEED_CURVE_EPSILON) {
        isVelocityUpdateNeeded = false;
    }
    const Vector2f newPos = lerp(startPosition, targetPosf, progress);
    return (newPos - currentPosition.as<f32>()).len() * TEXELS_PER_TILE;  // idfk why this works
}

f32 RailsControl::getSpeedNew() {
    // this isn't working for some reason
    // these can be calced once per segment
    const Vector2f targetPosf = getTarget().position.as<f32>();
    const f32 segmentDistance = (targetPosf - startPosition).len();
    const f32 expectedSegmentTime = segmentDistance / (speed * FPIXELS_PER_TEXEL);  // speed is in tiles/sec, but pos is in pixels

    f32 t = curActionTime / expectedSegmentTime;
    f32 newSpeed;
    switch (getTarget().movement) {
    case Movement::LINEAR:
        newSpeed = speed;
        break;

    case Movement::EASEIO_SINE:
        newSpeed = easeInOutSineDt(0, speed, t);
        break;

    case Movement::EASEIO_BEZIER:
        newSpeed = easeInOutBezierDt(0, speed, t);
        break;

    case Movement::EASEI_QUAD:
        newSpeed = easeInQuadDt(0, speed, t);
        break;

    case Movement::EASEI_CUBE:
        newSpeed = easeInCubicDt(0, speed, t);
        break;

    case Movement::EASEO_QUAD:
        newSpeed = easeOutQuad(0, 1, t);
        break;

    case Movement::EASEO_CUBE:
        newSpeed = easeOutCubic(0, 1, t);
        break;
    }

    return newSpeed;
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

void RailsControl::prepareForFirstStep(Transform2D& trans) {
    if (isValid()) {
        // set transform to match starting checkpoint
        Vector2i target = getTarget().position;
        startPosition = target.as<f32>();
        trans.position = target;
    } else {
        // o.w., make sure start position matches transform
        startPosition = trans.position.as<f32>();
    }
}

}  // namespace whal
