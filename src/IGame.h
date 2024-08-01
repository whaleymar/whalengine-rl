#pragma once

namespace whal {

class IGame {
public:
    virtual bool start() = 0;
    virtual void mainloop() = 0;
    virtual void end() = 0;
};

}  // namespace whal
