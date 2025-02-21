#include "Vector.h"

#include <ostream>

using std::ostream;

template <typename T>
std::ostream& operator<<(std::ostream& out, Vector2<T> const& self) {
    return out << "(" << self.x << ", " << self.y << ")";
}

#define DEF_VECTOR_IMPLS(type)                                                                                                                       \
    template ostream& operator<<(std::ostream& out, Vector2<type> const& self);                                                                      \
    template <>                                                                                                                                      \
    const Vector2<type> Vector2<type>::UP = Vector2<type>(0, 1);                                                                                     \
    template <>                                                                                                                                      \
    const Vector2<type> Vector2<type>::DOWN = Vector2<type>(0, -1);                                                                                  \
    template <>                                                                                                                                      \
    const Vector2<type> Vector2<type>::LEFT = Vector2<type>(-1, 0);                                                                                  \
    template <>                                                                                                                                      \
    const Vector2<type> Vector2<type>::RIGHT = Vector2<type>(1, 0);                                                                                  \
    template <>                                                                                                                                      \
    const Vector2<type> Vector2<type>::ZERO = Vector2<type>(0, 0);                                                                                   \
    template <>                                                                                                                                      \
    const Vector2<type> Vector2<type>::ONE = Vector2<type>(1, 1);

DEF_VECTOR_IMPLS(s8)
DEF_VECTOR_IMPLS(s16)
DEF_VECTOR_IMPLS(s32)
DEF_VECTOR_IMPLS(s64)
DEF_VECTOR_IMPLS(f32)
DEF_VECTOR_IMPLS(f64)
