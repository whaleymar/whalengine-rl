#include "Material.h"

#include <cmath>

#include "Components/Collision.h"
#include "Components/Draw.h"
#include "Components/Lifetime.h"
#include "Components/Light.h"
#include "Components/RigidBody.h"
#include "Components/Transform.h"
#include "Sys/System.h"
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
        entity.add(PointLight{halfLenTexels * 2});
    }

    if (isFlagSet(DecaySpeed)) {
        // want to add this one after some delay, in case particle gains speed in first few frames (like from gravity or something)
        System::schedule.eventFlow({entity}).addWait(0.5).add(
            [](ecs::Entity e, f32 minSpeedTPS, f32 decaySeconds, Color color, Color fadeColor) {
                e.add(DieWhenSpeedBelow(minSpeedTPS, ColorLerp(color, fadeColor, decaySeconds)));
            },
            entity, decayParams.decaySpeed.minSpeedTPS, decayParams.decaySpeed.decaySeconds, color, fadeColor);
    }

    if (isFlagSet(FadeOutFlag)) {
        // entity.add(FadeOut(lifetime));
        entity.add(ColorLerp(color, fadeColor, lifetime));
    }

    if (isFlagSet(RadianceFlag)) {
        entity.add(Radiance{2, 0, color});
    }

    if (startScale != 1.0) {
        entity.add(ScaleLerp(Vector2f(1.0, 1.0) * startScale, {1.0, 1.0}, lifetime / 2));
    }
}

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

static const MaterialData S_MATERIAL_GRASS = {
    .name = "Grass",
    .id = WorldMaterial::Grass,
    .colorRange = {DARKGREEN, GREEN},
    .flags = MaterialData::DecayTime | MaterialData::FadeOutFlag,
    .bounciness = 0.0,
    .gravityCoef = 0.0,
    .frictionCoefs = {0.0, 0.5},
    .decayParams = {.decayTime = {.decaySecondsMin = 5.0, .decaySecondsMax = 10.0}},
    .particleShape = Draw::DrawTag::Line,
};

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

static const MaterialData S_MATERIAL_DUST = {
    .name = "Dust",
    .id = WorldMaterial::Dust,
    .colorRange = {BEIGE, WHITE},
    .flags = MaterialData::DecayTime | MaterialData::FadeOutFlag,
    .bounciness = 0.0,
    .gravityCoef = 0.0,
    .frictionCoefs = {0.0, 0.0},
    .decayParams = {.decayTime = MaterialData::DecayTimeParams()},
    .startScale = 2.0,
};

static const MaterialData S_MATERIAL_FIRE = {.name = "Fire",
                                             .id = WorldMaterial::Fire,
                                             .colorRange = {ORANGE, RED},
                                             .flags = MaterialData::Light | MaterialData::RadianceFlag | MaterialData::DecayTime |
                                                      MaterialData::FadeOutFlag | MaterialData::RigidBodyFlag,
                                             .bounciness = 0.0,
                                             .gravityCoef = -0.5,
                                             .frictionCoefs = {0.0, 0.0},
                                             .fadeColor = BLACK,
                                             .decayParams = {.decayTime = MaterialData::DecayTimeParams(0.2, 0.5)}};

static const MaterialData S_MATERIAL_DEFAULT = S_MATERIAL_DUST;

static const MaterialData S_MATERIAL_EMBER = {
    .name = "Ember",
    .id = WorldMaterial::Ember,
    .colorRange = {ORANGE, RED},
    .flags = MaterialData::Light | MaterialData::RadianceFlag | MaterialData::DecaySpeed | MaterialData::FadeOutFlag | MaterialData::RigidBodyFlag |
             MaterialData::Collision,
    .bounciness = 1.0,
    .gravityCoef = 1.0,
    .frictionCoefs = {0.25, 0.0},
    .fadeColor = Color(ORANGE.r, ORANGE.g, ORANGE.b, 100),
    .decayParams = {.decaySpeed = MaterialData::DecaySpeedParams()},
};

static const MaterialData S_MATERIAL_POISON = {.name = "Ember",
                                               .id = WorldMaterial::Ember,
                                               // .colorRange = {RED, {Colors::Pink.r, Colors::Pink.g, Colors::Pink.b, 255}},
                                               .colorRange = {PINK, PINK},
                                               .flags = MaterialData::Light | MaterialData::RadianceFlag | MaterialData::DecaySpeed |
                                                        MaterialData::FadeOutFlag | MaterialData::RigidBodyFlag | MaterialData::Collision,
                                               .bounciness = 1.0,
                                               .gravityCoef = 1.0,
                                               .frictionCoefs = {0.25, 0.0},
                                               .decayParams = {.decaySpeed = MaterialData::DecaySpeedParams()}};

static const MaterialData S_MATERIAL_TINYDUST = {
    .name = "Dust",
    .id = WorldMaterial::TinyDust,
    .colorRange = {BEIGE, WHITE},
    .flags = MaterialData::DecayTime | MaterialData::FadeOutFlag,
    .bounciness = 0.0,
    .gravityCoef = 0.0,
    .frictionCoefs = {0.0, 0.0},
    .decayParams = {.decayTime = MaterialData::DecayTimeParams()},
};

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
    case WorldMaterial::TinyDust:
        return S_MATERIAL_TINYDUST;
    }
}

}  // namespace whal
