#pragma once

#include <iostream>
#include <sstream>

template <std::size_t N = 2, std::size_t M = 2>
struct Format {
    char sep[N] = " ";
    char end[M] = "\n";
};

template <Format f = Format({.sep = " ", .end = "\n"}), class T, class... U>
void print(const T& first, const U&... rest) {
    std::cout << first;
    ((std::cout << f.sep << rest), ...);
    std::cout << f.end;
}

template <class T, class... U>
std::string sprint(const T& first, const U&... rest) {
    std::stringstream stream;
    stream << first;
    ((stream << " " << rest), ...);
    return stream.str();
}

// just need this to compile fast
template <class T, class... U>
    requires(sizeof...(U) == 0)
std::string whal_format(const std::string& fmt, const T& first_, const U&... rest) {
    std::stringstream stream;
    stream << first_;
    std::string first = stream.str();
    size_t ix = fmt.find("{}");

    if (ix != std::string::npos) {
        int curIx = 0;
        std::string s = fmt.substr(curIx, ix - curIx);
        s += first;
        return s + std::string(fmt.substr(ix + 2));
    }
    return fmt;
}

template <class T, class... U>
    requires(sizeof...(U) > 0)
std::string whal_format(const std::string& fmt, const T& first_, const U&... rest) {
    std::stringstream stream;
    stream << first_;
    std::string first = stream.str();
    size_t ix = fmt.find("{}");

    if (ix != std::string::npos) {
        int curIx = 0;
        std::string s = fmt.substr(curIx, ix - curIx);
        s += first;
        return s + whal_format(fmt.substr(ix + 2), rest...);
    }
    return fmt;
}
