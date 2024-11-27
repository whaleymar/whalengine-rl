#pragma once

#include <memory>
#include <vector>

namespace whal {

class IShaderProcess;

// TODO i want a "pre-lighting" shader list
struct Camera {
    std::vector<std::shared_ptr<IShaderProcess>> postEffects;
    // std::vector<std::shared_ptr<IShaderProcess>> lightEffects;
};

}  // namespace whal
