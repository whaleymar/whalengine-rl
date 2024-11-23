#pragma once

#include <memory>
#include <vector>

namespace whal {

class IShaderProcess;

struct Camera {
    std::vector<std::shared_ptr<IShaderProcess>> postEffects;
    // std::vector<std::shared_ptr<IShaderProcess>> lightEffects;
};

}  // namespace whal
