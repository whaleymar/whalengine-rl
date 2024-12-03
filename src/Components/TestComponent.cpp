#include "TestComponent.h"

#include "rfl/fields.hpp"

namespace whal {

void TestCmp::loadImpl(ecs::Entity e, void* data) {
    // const LoadContext& ctx = *static_cast<LoadContext*>(data);
    print("running TestCmp::loadImpl");
    for (const auto& f : rfl::fields<TestCmp>()) {
        std::cout << "name: " << f.name() << ", type: " << f.type() << std::endl;
    }
}

}  // namespace whal
