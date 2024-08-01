#pragma once

namespace whal {

struct Scene;

class IGame {
public:
    virtual bool start() = 0;
    virtual void mainloop() = 0;
    virtual void end() = 0;
    virtual Scene& getScene() = 0;
};

}  // namespace whal
