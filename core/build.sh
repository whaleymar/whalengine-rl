#!/bin/sh
cmake . -DCMAKE_BUILD_TYPE=Debug -B build -G Ninja && cd build && cmake --build . -j && cd ..
