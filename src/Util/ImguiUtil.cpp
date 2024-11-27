#include "ImguiUtil.h"

namespace whal {

void ImguiMgr::draw() {
    for (IRenderDebug* pObj : instance().mObjs) {
        pObj->draw();
    }
}

IRenderDebug::IRenderDebug() {
    ImguiMgr::add(this);
}

IRenderDebug::~IRenderDebug() {
    ImguiMgr::remove(this);
}

}  // namespace whal
