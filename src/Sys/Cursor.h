#pragma once

namespace whal {

struct Sprite;
struct System;

class Cursor {
public:
    friend System;

    void setCursor(Sprite sprite) const;
    void setDefaultCursor() const;

private:
    Cursor() = default;
    Cursor(const Cursor&) = delete;
    void operator=(const Cursor&) = delete;
};

}  // namespace whal
