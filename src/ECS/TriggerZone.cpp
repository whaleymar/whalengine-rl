#include "TriggerZone.h"

namespace whal {

TriggerZone::TriggerZone(Transform transform, Vector2i halflen, TriggerCallback callbackEnter, TriggerCallback callbackExit)
    : AABB(transform, halflen), onTriggerEnter(callbackEnter), onTriggerExit(callbackExit){};

}
