#pragma once

namespace whal {

struct Scene;

// A game must implement the IGame interface and satisfy the Singleton and StaticUpdate concepts
class IGame {
public:
    virtual ~IGame() = default;
    virtual bool start() = 0;
    virtual void end() = 0;
    virtual Scene& getScene() = 0;
    virtual bool isSceneLoaded() const = 0;
};

}  // namespace whal
