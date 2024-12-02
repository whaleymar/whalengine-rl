#pragma once

namespace whal {

struct Scene;

class IGame {
public:
    virtual ~IGame() = default;
    virtual bool start() = 0;
    virtual void end() = 0;
    virtual Scene& getScene() = 0;
    virtual bool isSceneLoaded() const = 0;
};

}  // namespace whal
