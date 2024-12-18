#pragma once

#include <vector>

#include "Map/ComponentFactory.h"
#include "Util/Easing.h"
#include "Util/Vector.h"

namespace whal {

namespace ecs {
class Entity;
}

struct Transform;

struct RailsControl : ISerialize<RailsControl, ComponentFactory> {
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

    struct CheckPoint {
        Vector2i position;
        Ease movement;
    };

    using ArrivalCallback = void (*)(ecs::Entity, RailsControl&);

    RailsControl(f32 moveSpeed_ = 40, std::vector<CheckPoint> checkPoints_ = {}, f32 waitTime_ = 0,
                 CycleBehavior cycleBehavior_ = CycleBehavior::MANUAL_FIRSTSTEP_LOOP, ArrivalCallback callback = nullptr);

private:
    std::vector<CheckPoint> mCheckpoints;

public:
    f32 speed;
    f32 waitTime;
    ArrivalCallback arrivalCallback;

    Vector2f startPosition;
    f32 curActionTime = 0;  // time spent moving or waiting

    CycleBehavior endBehavior = CycleBehavior::MANUAL_FIRSTSTEP_LOOP;
    bool isWaiting = true;
    bool isVelocityUpdateNeeded = false;
    bool isForward = true;
    bool isPhysicsEntity;
    u32 curTarget = 0;

    void setCheckpoints(std::vector<CheckPoint>& checkpoints, Transform& trans, ecs::Entity e);
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
    void prepareForFirstStep(Transform& trans, ecs::Entity e);

    static void loadImpl(ecs::Entity entity, void* data);
};

}  // namespace whal
