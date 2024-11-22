#pragma once

#include <memory>
#include <vector>

namespace whal {

class IShader;

struct Camera {
    std::vector<std::shared_ptr<IShader>> postEffects;
    // std::vector<std::shared_ptr<BaseShader>> lightEffects;
};

}  // namespace whal
