#include "String.h"

#include <cstring>
#include <sstream>
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

// trim from start (in place)
inline static void ltrim(std::string& s) {
    s.erase(s.begin(), stl::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
}

// trim from end (in place)
inline static void rtrim(std::string& s) {
    s.erase(stl::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
}

std::string& strip_inplace(std::string& str) {
    rtrim(str);
    ltrim(str);
    return str;
}

std::string strip(const std::string& inpt) {
    std::string line = inpt;
    line.erase(line.begin(), stl::find_if(line.begin(), line.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    return line;
}

bool contains(const std::string& str, const std::string& substr) {
    return str.find(substr) != std::string::npos;
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::stringstream ss(str);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string splitAndGet(const std::string& str, char delimiter, size_t index) {
    auto tokens = split(str, delimiter);
    if (index < tokens.size()) {
        return tokens[index];
    } else {
        // IndexError
        return "";
    }
}

std::string replace(std::string str, const std::string& from, const std::string& to) {
    if (from.empty()) {
        return str;  // Avoid infinite loops on empty substring
    }
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();  // Move past the last replaced segment
    }
    return str;
}

}  // namespace whal
