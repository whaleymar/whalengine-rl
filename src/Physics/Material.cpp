#include "Material.h"
#include <cmath>

#include "Components/Collision.h"
#include "Components/Draw.h"
#include "Components/Lifetime.h"
#include "Components/Light.h"
#include "Components/RigidBody.h"
#include "Physics/Shapes.h"

namespace whal {

constexpr f32 MAX_LIFETIME_SECONDS = 60.0f;

static MaterialData getMaterialData(WorldMaterial material);
MaterialData MaterialData::get(WorldMaterial material) {
    return getMaterialData(material);
}

f32 MaterialData::getDecayTime() const {
    return isFlagSet(DecayTime) ? std::lerp(decayParams.decayTime.decaySecondsMin, decayParams.decayTime.decaySecondsMax, System::rng.uniform()) :
                                  MAX_LIFETIME_SECONDS;
}

Color MaterialData::getColor() const {
    return Colors::lerp(colorRange[0], colorRange[1], System::rng.uniform());
}

void MaterialData::addComponents(ecs::Entity entity, s32 halfLenTexels, Color color, f32 lifetimeMultiplier) const {
    const f32 lifetime = getDecayTime() * lifetimeMultiplier;
    entity.add(Lifetime(lifetime));

    if (isFlagSet(Collision)) {
        auto collider = Collider::Actor(AABB(entity.get<Transform2D>(), {halfLenTexels, halfLenTexels}));
        collider.setMaterial(id);
        entity.add(collider);
    }

    if (isFlagSet(RigidBodyFlag)) {
        auto rigidBody = RigidBody();
        rigidBody.gravityMultiplier = gravityCoef;
        rigidBody.frictionMultiplier = frictionCoefs;
        entity.add(rigidBody);
    }

    if (isFlagSet(Light)) {
        entity.add(PointLight{halfLenTexels * 4});
    }

    if (isFlagSet(RadianceFlag)) {
        entity.add(Radiance{halfLenTexels * 4, 0, color});
    }

    if (isFlagSet(DecaySpeed)) {
        entity.add(DieWhenSpeedBelow(decayParams.decaySpeed.minSpeedTPS, decayParams.decaySpeed.decaySeconds, isFlagSet(FadeOutFlag)));
    }

    if (isFlagSet(FadeOutFlag)) {
        entity.add(FadeOut(lifetime));
    }
}

static const MaterialData S_MATERIAL_DEFAULT = {.name = "Default",
                                                .id = WorldMaterial::None,
                                                .colorRange = {WHITE, WHITE},
                                                .flags = MaterialData::DecayTime | MaterialData::FadeOutFlag,
                                                .bounciness = 0.0,
                                                .gravityCoef = 0.0,
                                                .frictionCoefs = {0.0, 0.0},
                                                .decayParams = {.decayTime = MaterialData::DecayTimeParams()}};

static const MaterialData S_MATERIAL_DIRT = {.name = "Dirt",
                                             .id = WorldMaterial::Dirt,
                                             .colorRange = {DARKBROWN, BROWN},
                                             .flags = MaterialData::DecayTime | MaterialData::FadeOutFlag,
                                             .bounciness = 0.0,
                                             .gravityCoef = -0.5,
                                             .frictionCoefs = {0.0, 0.0},
                                             .decayParams = {.decayTime = MaterialData::DecayTimeParams()}};

static const MaterialData S_MATERIAL_ROCK = {.name = "Rock",
                                             .id = WorldMaterial::Rock,
                                             .colorRange = {DARKGRAY, GRAY},
                                             .flags = MaterialData::DecaySpeed | MaterialData::Collision | MaterialData::RigidBodyFlag,
                                             .bounciness = 0.0,
                                             .gravityCoef = 1.0,
                                             .frictionCoefs = {1.0, 1.0},
                                             .decayParams = {.decaySpeed = MaterialData::DecaySpeedParams()}};

static const MaterialData S_MATERIAL_SOFT = {.name = "Soft",
                                             .id = WorldMaterial::Soft,
                                             .colorRange = {BEIGE, WHITE},
                                             .flags = MaterialData::DecayTime | MaterialData::FadeOutFlag,
                                             .bounciness = 0.0,
                                             .gravityCoef = 1.0,
                                             .frictionCoefs = {0.0, 0.0},
                                             .decayParams = {.decayTime = MaterialData::DecayTimeParams()}};

static const MaterialData S_MATERIAL_WOOD = {.name = "Wood",
                                             .id = WorldMaterial::Wood,
                                             .colorRange = {DARKBROWN, BEIGE},
                                             .flags = MaterialData::DecaySpeed | MaterialData::Collision | MaterialData::RigidBodyFlag,
                                             .bounciness = 0.25,
                                             .gravityCoef = 1.0,
                                             .frictionCoefs = {1.0, 1.0},
                                             .decayParams = {.decaySpeed = MaterialData::DecaySpeedParams()}};

static const MaterialData S_MATERIAL_GRASS = {.name = "Grass",
                                              .id = WorldMaterial::Grass,
                                              .colorRange = {DARKGREEN, GREEN},
                                              .flags = MaterialData::DecayTime | MaterialData::FadeOutFlag,
                                              .bounciness = 0.0,
                                              .gravityCoef = 0.0,
                                              .frictionCoefs = {0.0, 0.0},
                                              .decayParams = {.decayTime = MaterialData::DecayTimeParams()}};

static const MaterialData S_MATERIAL_WATER = {.name = "Water",
                                              .id = WorldMaterial::Water,
                                              .colorRange = {DARKBLUE, Colors::LightBlue},
                                              .flags = MaterialData::Liquid | MaterialData::Collision | MaterialData::RigidBodyFlag |
                                                       MaterialData::DecayTime | MaterialData::FadeOutFlag,
                                              .bounciness = 0.0,
                                              .gravityCoef = 1.0,
                                              .frictionCoefs = {1.0, 1.0},
                                              .decayParams = {.decayTime = MaterialData::DecayTimeParams()}};

static const MaterialData S_MATERIAL_METAL = {.name = "Metal",
                                              .id = WorldMaterial::Metal,
                                              .colorRange = {DARKGRAY, GRAY},
                                              .flags = MaterialData::DecaySpeed | MaterialData::Collision | MaterialData::RigidBodyFlag,
                                              .bounciness = 0.0,
                                              .gravityCoef = 1.0,
                                              .frictionCoefs = {1.0, 1.0},
                                              .decayParams = {.decaySpeed = MaterialData::DecaySpeedParams()}};

static const MaterialData S_MATERIAL_RUBBER = {.name = "Rubber",
                                               .id = WorldMaterial::Rubber,
                                               .colorRange = {DARKGRAY, BLACK},
                                               .flags = MaterialData::DecayTime | MaterialData::Collision | MaterialData::RigidBodyFlag,
                                               .bounciness = 1.0,
                                               .gravityCoef = 1.0,
                                               .frictionCoefs = {1.0, 1.0},
                                               .decayParams = {.decayTime = MaterialData::DecayTimeParams()}};

static const MaterialData S_MATERIAL_DUST = {.name = "Dust",
                                             .id = WorldMaterial::Dust,
                                             .colorRange = {BEIGE, WHITE},
                                             .flags = MaterialData::DecayTime | MaterialData::FadeOutFlag,
                                             .bounciness = 0.0,
                                             .gravityCoef = 0.0,
                                             .frictionCoefs = {0.0, 0.0},
                                             .decayParams = {.decayTime = MaterialData::DecayTimeParams()}};

static const MaterialData S_MATERIAL_FIRE = {.name = "Fire",
                                             .id = WorldMaterial::Fire,
                                             .colorRange = {ORANGE, RED},
                                             .flags = MaterialData::Light | MaterialData::RadianceFlag | MaterialData::DecayTime |
                                                      MaterialData::FadeOutFlag | MaterialData::RigidBodyFlag,
                                             .bounciness = 0.0,
                                             .gravityCoef = -0.5,
                                             .frictionCoefs = {0.0, 0.0},
                                             .decayParams = {.decayTime = MaterialData::DecayTimeParams(0.2, 0.5)}};

static const MaterialData S_MATERIAL_EMBER = {.name = "Ember",
                                              .id = WorldMaterial::Ember,
                                              .colorRange = {ORANGE, RED},
                                              .flags = MaterialData::Light | MaterialData::RadianceFlag | MaterialData::DecaySpeed |
                                                       MaterialData::FadeOutFlag | MaterialData::RigidBodyFlag | MaterialData::Collision,
                                              .bounciness = 1.0,
                                              .gravityCoef = 1.0,
                                              .frictionCoefs = {0.25, 0.0},
                                              .decayParams = {.decaySpeed = MaterialData::DecaySpeedParams()}};

static const MaterialData S_MATERIAL_POISON = {
    .name = "Ember",
    .id = WorldMaterial::Ember,
    .colorRange = {Color(Colors::Pink.r, Colors::Pink.g, Colors::Pink.b, 125), Color(Colors::Pink.r, Colors::Pink.g, Colors::Pink.b, 40)},
    .flags = MaterialData::Light | MaterialData::RadianceFlag | MaterialData::DecaySpeed | MaterialData::FadeOutFlag | MaterialData::RigidBodyFlag |
             MaterialData::Collision,
    .bounciness = 1.0,
    .gravityCoef = 1.0,
    .frictionCoefs = {0.25, 0.0},
    .decayParams = {.decaySpeed = MaterialData::DecaySpeedParams()}};

MaterialData getMaterialData(WorldMaterial material) {
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
    }
}

}  // namespace whal
