#include "Material.h"

namespace whal::WhalMaterial {

const char* toString(WorldMaterial material) {
    switch (material) {
    case WorldMaterial::None:
        return "None";

    case WorldMaterial::Dirt:
        return "Dirt";

    case WorldMaterial::Rock:
        return "Rock";

    case WorldMaterial::Soft:
        return "Soft";

    case WorldMaterial::Wood:
        return "Wood";

    case WorldMaterial::Grass:
        return "Grass";

    case WorldMaterial::Water:
        return "Water";

    case WorldMaterial::Metal:
        return "Metal";
    }
}

f32 bounciness(WorldMaterial material) {
    switch (material) {
    case WorldMaterial::None:
        return 0.0;

    case WorldMaterial::Dirt:
        return 0.1;

    case WorldMaterial::Rock:
        return 0.2;

    case WorldMaterial::Soft:
        return 0.5;

    case WorldMaterial::Wood:
        return 0.1;

    case WorldMaterial::Grass:
        return 0.1;

    case WorldMaterial::Water:
        return 0.0;

    case WorldMaterial::Metal:
        return 0.0;
    }
}

}  // namespace whal::WhalMaterial
