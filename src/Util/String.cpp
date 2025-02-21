#include "String.h"

#include <cstring>
#include "STL_reduce.h"

namespace whal {

bool isEqualString(const char* left, const char* right) {
    return strcmp(left, right) == 0;
}

bool isEqualString(const std::string& left, const std::string& right) {
    return left == right;
}

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.compare(0, prefix.size(), prefix) == 0;
}

std::string strip(const std::string& inpt) {
    std::string line = inpt;
    line.erase(line.begin(), stl::find_if(line.begin(), line.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    return line;
}

bool contains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

}  // namespace whal
