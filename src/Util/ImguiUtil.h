#pragma once

#ifndef NDEBUG
#include <unordered_set>

namespace whal {

class IRenderDebug;
class ImguiMgr {
public:
    static ImguiMgr& instance() {
        static ImguiMgr instance_;
        return instance_;
    }

    static void add(IRenderDebug* obj) { instance().mObjs.insert(obj); }
    static void remove(IRenderDebug* obj) { instance().mObjs.erase(obj); };
    static void draw();

private:
    std::unordered_set<IRenderDebug*> mObjs;
};

// make sure an empty class definition exists for inheritance reasons
class IRenderDebug {
public:
    virtual ~IRenderDebug();
    virtual void draw() = 0;

protected:
    IRenderDebug();
};

}  // namespace whal
#endif
