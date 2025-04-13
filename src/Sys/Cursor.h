#pragma once

#include "Util/Singleton.h"
namespace whal {

struct Sprite;

class CursorManager {
    SINGLETON(CursorManager)
public:
    void set(Sprite sprite) const;
    void set(const char* spritePath) const;
    void setDefault() const;
};

}  // namespace whal
