#include "Material.h"

namespace whal {

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

}  // namespace whal
