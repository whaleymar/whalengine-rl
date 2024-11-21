#pragma once

#include <memory>
#include <vector>

namespace whal {

class BaseShader;

struct Camera {
    std::vector<std::shared_ptr<BaseShader>> postEffects;
    // std::vector<std::shared_ptr<BaseShader>> lightEffects;
};

}  // namespace whal
