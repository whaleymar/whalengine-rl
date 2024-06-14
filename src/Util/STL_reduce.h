#pragma once

namespace whal::std {

// std::find from <algorithm> so I don't have to include the whole thing
template <class InputIterator, class T>
InputIterator find(InputIterator first, InputIterator last, const T& val) {
    while (first != last) {
        if (*first == val)
            return first;
        ++first;
    }
    return last;
}

}  // namespace whal::std
