#pragma once

#include "Map/ComponentFactory.h"
#include "Util/Types.h"

namespace whal {

struct TestCmp : ISerialize<TestCmp, ComponentFactoryNew> {
    s32 myInt;
    f32 myFloat;
    const char* myCString;
    Vector2i myVec;

    static void loadImpl(ecs::Entity e, void* data);

    static void* saveImpl(ecs::Entity e) {
        print("running TestCmp::saveImpl");
        return nullptr;
    }
};

// constexpr bool SB = CustomLoad<TestCmp>;
// constexpr bool SB2 = CustomSave<TestCmp>;

REGISTER_COMPONENT(TestCmp)

}  // namespace whal
