#include "Vector.h"

#include <ostream>

using std::ostream;

template <typename T>
std::ostream& operator<<(std::ostream& out, Vector2<T> const& self) {
    return out << "(" << self.x << ", " << self.y << ")";
}

// DECLARE ALL INSTANTIATIONS OF VECTOR (that i want to print)
template ostream& operator<<(std::ostream& out, Vector2<s32> const& self);
template ostream& operator<<(std::ostream& out, Vector2<f32> const& self);
