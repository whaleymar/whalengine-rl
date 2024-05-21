#include "Name.h"

namespace whal {

std::ostream& operator<<(std::ostream& out, Name const& self) {
    return out << self.name;
}

}  // namespace whal
