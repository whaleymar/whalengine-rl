#include "Material.h"

namespace whal {

const char* toString(Material material) {
    switch (material) {
    case Material::None:
        return "None";

    case Material::Dirt:
        return "Dirt";

    case Material::Rock:
        return "Rock";

    case Material::Soft:
        return "Soft";

    case Material::Wood:
        return "Wood";

    case Material::Grass:
        return "Grass";

    case Material::Water:
        return "Water";

    case Material::Metal:
        return "Metal";
    }
}

}  // namespace whal
