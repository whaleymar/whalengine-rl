#pragma once

#include <vector>

#include "Util/Vector.h"

namespace whal {

namespace ecs {
class Entity;
}

struct Transform2D;

struct RailsControl {
    enum class EndBehavior : u8 { TO_START, REVERSE };
    enum class Movement { LINEAR, EASEIO_BEZIER, EASEIO_SINE, EASEI_QUAD, EASEI_CUBE, EASEO_QUAD, EASEO_CUBE };

    struct CheckPoint {
        Vector2i position;
        Movement movement;
    };

    using ArrivalCallback = void (*)(ecs::Entity, RailsControl&);

    RailsControl(f32 moveSpeed_ = 40, std::vector<CheckPoint> checkPoints_ = {}, f32 waitTime_ = 0, bool isCycle_ = true,
                 ArrivalCallback callback = nullptr);

private:
    std::vector<CheckPoint> mCheckpoints;

public:
    f32 speed;  // texels per second
    f32 waitTime;
    ArrivalCallback arrivalCallback;

    Vector2f startPosition;
    f32 curActionTime = 0;  // time spent moving or waiting

    // TODO should rework this, combined with EndBehavior enum, to allow for indefinite waiting at each checkpoint
    bool isCycle;  // if true, repeats after returning to first checkpoint
    EndBehavior endBehavior;
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
    void prepareForFirstStep(Transform2D& trans);
};

}  // namespace whal
