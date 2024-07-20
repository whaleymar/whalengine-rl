#pragma once

#include <raylib.h>

#include "Util/Types.h"
#include "Util/Vector.h"

namespace whal {

enum class WorldMaterial : u8 { None, Dirt, Rock, Soft, Wood, Grass, Water, Metal, Rubber, Dust, Fire, Ember, Poison, TinyDust };

namespace ecs {
class Entity;
}

struct MaterialData {
    enum Flags : u8 {
        None = 0,
        Collision = 1,
        RigidBodyFlag = 1 << 1,
        Liquid = 1 << 2,
        Light = 1 << 3,
        RadianceFlag = 1 << 4,
        DecayTime = 1 << 5,
        DecaySpeed = 1 << 6,
        FadeOutFlag = 1 << 7,
    };

    struct DecayTimeParams {
        f32 decaySecondsMin = 0.5;
        f32 decaySecondsMax = 2.0;
    };

    struct DecaySpeedParams {
        f32 minSpeedTPS = 0.01;
        f32 decaySeconds = 0.25;
    };

    static MaterialData get(WorldMaterial material);
    bool isFlagSet(Flags flag) const { return (flags & flag) > 0; }
    f32 getDecayTime() const;
    Color getColor() const;
    void addComponents(ecs::Entity entity, s32 halfLenTexels, Color color, f32 lifetimeMultiplier = 1.0) const;

    const char* name;
    WorldMaterial id;
    Color colorRange[2];
    u8 flags = DecayTime | FadeOutFlag;
    f32 bounciness = 0.0;
    f32 gravityCoef = 1.0;
    Vector2f frictionCoefs = {1.0, 1.0};
    Color fadeColor = Color(255, 255, 255, 0);
    union {
        DecayTimeParams decayTime;
        DecaySpeedParams decaySpeed;
    } decayParams;
    f32 startScale = 1.0;
};

}  // namespace whal
