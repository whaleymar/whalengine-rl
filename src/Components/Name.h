#pragma once

#include <ostream>

#include "Util/String.h"

namespace whal {

struct Name {
    std::string name;

    bool operator==(Name& other) const { return isEqualString(name, other.name); }
};

std::ostream& operator<<(std::ostream& out, Name const& self);

}  // namespace whal
