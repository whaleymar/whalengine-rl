#include "MaterialData.h"

#include <cmath>

#include "Components/Draw.h"
#include "Sys/System.h"

namespace whal {

constexpr f32 MAX_LIFETIME_SECONDS = 60.0f;

static const MaterialData& getMaterialData(WorldMaterial material);
const MaterialData& MaterialData::get(WorldMaterial material) {
    return getMaterialData(material);
}

f32 MaterialData::getDecayTime() const {
    return isFlagSet(DecayTime) ? std::lerp(decayParams.decayTime.decaySecondsMin, decayParams.decayTime.decaySecondsMax, Rng.uniform()) :
                                  MAX_LIFETIME_SECONDS;
}

Color MaterialData::getColor() const {
    return Color::lerp(colorRange[0], colorRange[1], Rng.uniform());
}

static const MaterialData S_MATERIAL_DIRT = {
    .name = "Dirt",
    .id = WorldMaterial::Dirt,
    .colorRange = {Colors::DarkBrown, Colors::Brown},
    .flags = MaterialData::DecayTime | MaterialData::FadeOutFlag,
    .bounciness = 0.0,
    .gravityCoef = -0.5,
    .frictionCoefs = {0.0, 0.0},
    .decayParams = {.decayTime = MaterialData::DecayTimeParams()},
};

static const MaterialData S_MATERIAL_ROCK = {
    .name = "Rock",
    .id = WorldMaterial::Rock,
    .colorRange = {Colors::DarkGray, Colors::Gray},
    .flags = MaterialData::DecaySpeed | MaterialData::Collision | MaterialData::RigidBodyFlag,
    .bounciness = 0.0,
    .gravityCoef = 1.0,
    .frictionCoefs = {1.0, 1.0},
    .decayParams = {.decaySpeed = MaterialData::DecaySpeedParams()},
};

static const MaterialData S_MATERIAL_SOFT = {
    .name = "Soft",
    .id = WorldMaterial::Soft,
    .colorRange = {Colors::Beige, Colors::White},
    .flags = MaterialData::DecayTime | MaterialData::FadeOutFlag,
    .bounciness = 0.0,
    .gravityCoef = 1.0,
    .frictionCoefs = {0.0, 0.0},
    .decayParams = {.decayTime = MaterialData::DecayTimeParams()},
};

static const MaterialData S_MATERIAL_WOOD = {
    .name = "Wood",
    .id = WorldMaterial::Wood,
    .colorRange = {Colors::DarkBrown, Colors::Beige},
    .flags = MaterialData::DecaySpeed | MaterialData::Collision | MaterialData::RigidBodyFlag,
    .bounciness = 0.25,
    .gravityCoef = 1.0,
    .frictionCoefs = {1.0, 1.0},
    .decayParams = {.decaySpeed = MaterialData::DecaySpeedParams()},
};

static const MaterialData S_MATERIAL_GRASS = {
    .name = "Grass",
    .id = WorldMaterial::Grass,
    .colorRange = {Colors::DarkGreen, Colors::Green},
    .flags = MaterialData::DecayTime | MaterialData::FadeOutFlag | MaterialData::RandomSpinDir,
    .bounciness = 0.0,
    .gravityCoef = 0.0,
    .frictionCoefs = {0.0, 0.5},
    .decayParams = {.decayTime = {.decaySecondsMin = 5.0, .decaySecondsMax = 10.0}},
    .particleShape = DrawTag::Line,
    .minRotationsPerSec = 0.25f,
    .maxRotationsPerSec = 2.0f,
};

static const MaterialData S_MATERIAL_WATER = {
    .name = "Water",
    .id = WorldMaterial::Water,
    .colorRange = {Colors::DarkBlue, Colors::LightBlue},
    .flags = MaterialData::Liquid | MaterialData::Collision | MaterialData::RigidBodyFlag | MaterialData::DecayTime | MaterialData::FadeOutFlag,
    .bounciness = 0.0,
    .gravityCoef = 1.0,
    .frictionCoefs = {1.0, 1.0},
    .decayParams = {.decayTime = MaterialData::DecayTimeParams()},
};

static const MaterialData S_MATERIAL_METAL = {
    .name = "Metal",
    .id = WorldMaterial::Metal,
    .colorRange = {Colors::DarkGray, Colors::Gray},
    .flags = MaterialData::DecaySpeed | MaterialData::Collision | MaterialData::RigidBodyFlag,
    .bounciness = 0.0,
    .gravityCoef = 1.0,
    .frictionCoefs = {1.0, 1.0},
    .decayParams = {.decaySpeed = MaterialData::DecaySpeedParams()},
};

static const MaterialData S_MATERIAL_RUBBER = {
    .name = "Rubber",
    .id = WorldMaterial::Rubber,
    .colorRange = {Colors::DarkGray, Colors::Black},
    .flags = MaterialData::DecayTime | MaterialData::Collision | MaterialData::RigidBodyFlag,
    .bounciness = 1.0,
    .gravityCoef = 1.0,
    .frictionCoefs = {1.0, 1.0},
    .decayParams = {.decayTime = MaterialData::DecayTimeParams()},
};

static const MaterialData S_MATERIAL_DUST = {
    .name = "Dust",
    .id = WorldMaterial::Dust,
    .colorRange = {Colors::Beige, Colors::White},
    .flags = MaterialData::DecayTime | MaterialData::FadeOutFlag,
    .bounciness = 0.0,
    .gravityCoef = 0.0,
    .frictionCoefs = {0.0, 0.0},
    .decayParams = {.decayTime = MaterialData::DecayTimeParams()},
    .minScale = 0.0,
    .maxScale = 2.0,
};

static const MaterialData S_MATERIAL_FIRE = {
    .name = "Fire",
    .id = WorldMaterial::Fire,
    .colorRange = {Colors::Orange, Colors::Red},
    .flags = MaterialData::Light | MaterialData::DecayTime | MaterialData::FadeOutFlag | MaterialData::RigidBodyFlag,
    .bounciness = 0.0,
    .gravityCoef = -0.5,
    .frictionCoefs = {0.0, 0.0},
    .fadeColor = Colors::Clear,
    .decayParams = {.decayTime = MaterialData::DecayTimeParams(0.4, 0.6)},
    .brightness = 2.0,
};

static const MaterialData S_MATERIAL_DEFAULT = S_MATERIAL_DUST;

static const MaterialData S_MATERIAL_EMBER = {
    .name = "Ember",
    .id = WorldMaterial::Ember,
    .colorRange = {Colors::Orange, Colors::Red},
    .flags = MaterialData::Light | MaterialData::DecaySpeed | MaterialData::FadeOutFlag | MaterialData::RigidBodyFlag | MaterialData::Collision,
    .bounciness = 0.7,
    .gravityCoef = 1.0,
    .frictionCoefs = {0.25, 0.02},
    .fadeColor = Color(Colors::Orange.r, Colors::Orange.g, Colors::Orange.b, 0.25),
    .decayParams = {.decaySpeed = MaterialData::DecaySpeedParams()},
    .brightness = 1.5,
};

static const MaterialData S_MATERIAL_POISON = {
    .name = "Ember",
    .id = WorldMaterial::Ember,
    .colorRange = {Colors::Pink, Colors::Pink},
    .flags = MaterialData::Light | MaterialData::DecaySpeed | MaterialData::FadeOutFlag | MaterialData::RigidBodyFlag | MaterialData::Collision,
    .bounciness = 0.7,
    .gravityCoef = 1.0,
    .frictionCoefs = {0.25, 0.02},
    .decayParams = {.decaySpeed = MaterialData::DecaySpeedParams()},
    .brightness = 2.0,
};

static const MaterialData S_MATERIAL_TINYDUST = {
    .name = "Dust",
    .id = WorldMaterial::TinyDust,
    .colorRange = {Colors::Beige, Colors::White},
    .flags = MaterialData::DecayTime | MaterialData::FadeOutFlag,
    .bounciness = 0.0,
    .gravityCoef = 0.0,
    .frictionCoefs = {0.0, 0.0},
    .decayParams = {.decayTime = MaterialData::DecayTimeParams()},
};

static const MaterialData S_MATERIAL_MAGIK = {
    .name = "Magik",
    .id = WorldMaterial::Magik,
    .colorRange = {Colors::Purple, Colors::DarkBlue},
    // .flags = MaterialData::DecayTime | MaterialData::ScaleUp | MaterialData::ScaleBounce | MaterialData::RandomSpinDir,
    .flags = MaterialData::DecayTime | MaterialData::RandomSpinDir,
    .bounciness = 0.0,
    .gravityCoef = 0.0,
    .frictionCoefs = {0.0, 0.0},
    .decayParams = {.decayTime = MaterialData::DecayTimeParams(0.25, 0.5)},
    .minScale = 0.0,
    .maxScale = 3.0,
    .minRotationsPerSec = 0.25f,
    .maxRotationsPerSec = 1.5f,
};

const MaterialData& getMaterialData(WorldMaterial material) {
    switch (material) {
    case WorldMaterial::None:
        return S_MATERIAL_DEFAULT;
    case WorldMaterial::Dirt:
        return S_MATERIAL_DIRT;
    case WorldMaterial::Rock:
        return S_MATERIAL_ROCK;
    case WorldMaterial::Soft:
        return S_MATERIAL_SOFT;
    case WorldMaterial::Wood:
        return S_MATERIAL_WOOD;
    case WorldMaterial::Grass:
        return S_MATERIAL_GRASS;
    case WorldMaterial::Water:
        return S_MATERIAL_WATER;
    case WorldMaterial::Metal:
        return S_MATERIAL_METAL;
    case WorldMaterial::Rubber:
        return S_MATERIAL_RUBBER;
    case WorldMaterial::Dust:
        return S_MATERIAL_DUST;
    case WorldMaterial::Fire:
        return S_MATERIAL_FIRE;
    case WorldMaterial::Ember:
        return S_MATERIAL_EMBER;
    case WorldMaterial::Poison:
        return S_MATERIAL_POISON;
    case WorldMaterial::TinyDust:
        return S_MATERIAL_TINYDUST;
    case WorldMaterial::Magik:
        return S_MATERIAL_MAGIK;
    }
}

}  // namespace whal
