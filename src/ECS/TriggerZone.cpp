#include "TriggerZone.h"

namespace whal {

Trigger::Trigger(Shape shape_, TriggerCallback callbackEnter, TriggerCallback callbackExit)
    : shape(shape_), onTriggerEnter(callbackEnter), onTriggerExit(callbackExit) {}

// TriggerZone::TriggerZone(Transform2D transform, Vector2i halflen, TriggerCallback callbackEnter, TriggerCallback callbackExit)
//     : AABB2(transform, halflen), onTriggerEnter(callbackEnter), onTriggerExit(callbackExit) {}
//
// TriggerCircle::TriggerCircle(Transform2D transform, s32 radius, TriggerCallback callbackEnter, TriggerCallback callbackExit)
//     : Circle2(transform, radius), onTriggerEnter(callbackEnter), onTriggerExit(callbackExit) {}

}  // namespace whal
