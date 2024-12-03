#pragma once

namespace whal::stl {

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

template <class InputIt, class UnaryPred>
constexpr InputIt find_if(InputIt first, InputIt last, UnaryPred p) {
    for (; first != last; ++first)
        if (p(*first))
            return first;

    return last;
}

}  // namespace whal::stl
