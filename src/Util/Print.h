#pragma once

#include <iostream>

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

template <typename T>
void printArray(T* array, int len) {
    for (int i = 0; i < len; i++) {
        std::cout << array[i] << std::endl;
    }
}
