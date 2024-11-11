#pragma once

namespace whal {

struct Sprite;
struct System;

class CursorManager {
public:
    friend System;

    CursorManager() = default;
    void set(Sprite sprite) const;
    void setDefault() const;

private:
    CursorManager(const CursorManager&) = delete;
    void operator=(const CursorManager&) = delete;
};

}  // namespace whal
