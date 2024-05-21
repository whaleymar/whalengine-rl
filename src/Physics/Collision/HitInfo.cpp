#include "Physics/Collision/HitInfo.h"
#include "Util/Vector.h"

namespace whal {

HitInfo::HitInfo() {}

HitInfo::HitInfo(Vector2i normal) : normal(normal){};
}  // namespace whal
