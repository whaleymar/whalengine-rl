#include "String.h"

#include <cstring>

namespace whal {

bool isEqualString(const char* left, const char* right) {
    return strcmp(left, right) == 0;
}

bool isEqualString(const std::string& left, const std::string& right) {
    return left == right;
}

}  // namespace whal
