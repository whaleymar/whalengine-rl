#!/usr/bin/env bash 

echo "#include <$1>" | clang++ -std=c++20 -E -x c++ - | wc -l
