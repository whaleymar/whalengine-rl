#pragma once

#include <vector>

#include "Util/Vector.h"

namespace whal {

namespace ecs {
class Entity;
}

struct Transform2D;

struct RailsControl {
    // Automatic: moves by itself
    // Manual_FirstStep: requires manual start, then moves by itself through all checkpoints and waits at the start again
    // Manual_AllSteps: requires manual start at each checkpoint
    enum class CycleBehavior : u8 {
        AUTOMATIC_LOOP,
        AUTOMATIC_BACKTRACK,
        MANUAL_FIRSTSTEP_LOOP,
        MANUAL_FIRSTSTEP_BACKTRACK,
        MANUAL_ALLSTEPS_LOOP,
        MANUAL_ALLSTEPS_BACKTRACK,
    };

    enum class Movement { LINEAR, EASEIO_BEZIER, EASEIO_SINE, EASEI_QUAD, EASEI_CUBE, EASEO_QUAD, EASEO_CUBE };

    struct CheckPoint {
        Vector2i position;
        Movement movement;
    };

    using ArrivalCallback = void (*)(ecs::Entity, RailsControl&);

    RailsControl(f32 moveSpeed_ = 40, std::vector<CheckPoint> checkPoints_ = {}, f32 waitTime_ = 0,
                 CycleBehavior cycleBehavior_ = CycleBehavior::MANUAL_FIRSTSTEP_LOOP, ArrivalCallback callback = nullptr);

private:
    std::vector<CheckPoint> mCheckpoints;

public:
    f32 speed;  // texels per second
    f32 waitTime;
    ArrivalCallback arrivalCallback;

    Vector2f startPosition;
    f32 curActionTime = 0;  // time spent moving or waiting

    CycleBehavior endBehavior = CycleBehavior::MANUAL_FIRSTSTEP_LOOP;
    bool isWaiting = true;
    bool isVelocityUpdateNeeded = false;
    bool isForward = true;
    u64 curTarget = 0;

    void setCheckpoints(std::vector<CheckPoint>& checkpoints, Transform2D& trans);
    CheckPoint getTarget() const;
    void startManually();
    void step();
    f32 getSpeed(Vector2i currentPosition);
    f32 getSpeedNew();
    bool isValid() const;
    bool isLooping() const;
    bool isNextStepAutomatic() const;
    bool isAtFirstCheckpoint() const { return curTarget == 0; }
    bool isAtLastCheckpoint() const { return curTarget == (mCheckpoints.size() - 1); }
    void prepareForFirstStep(Transform2D& trans);
};

}  // namespace whal
