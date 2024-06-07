#include "TriggerZone.h"

#include "ECS/Transform.h"

namespace whal {

// Trigger::Trigger(IColliderShape* shape_, TriggerCallback callbackEnter, TriggerCallback callbackExit)
//     : shape(shape_->clone()), onTriggerEnter(callbackEnter), onTriggerExit(callbackExit) {}

TriggerZone::TriggerZone(Transform2D transform, Vector2i halflen, TriggerCallback callbackEnter, TriggerCallback callbackExit)
    : AABB2(transform, halflen, CollisionLayer::Trigger), onTriggerEnter(callbackEnter), onTriggerExit(callbackExit) {}

TriggerCircle::TriggerCircle(Transform2D transform, s32 radius, TriggerCallback callbackEnter, TriggerCallback callbackExit)
    : Circle2(transform, radius, CollisionLayer::Trigger), onTriggerEnter(callbackEnter), onTriggerExit(callbackExit) {}

}  // namespace whal
