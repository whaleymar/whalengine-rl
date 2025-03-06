#pragma once

#include <string>

namespace whal {

bool isEqualString(const char* left, const char* right);
bool isEqualString(const std::string& left, const std::string& right);

bool startsWith(const std::string& str, const std::string& prefix);

std::string strip(const std::string& str);

bool contains(const std::string& str, const std::string& substr);
std::string splitAndGet(const std::string& str, char delimiter, size_t index);
std::string replace(std::string str, const std::string& from, const std::string& to);

}  // namespace whal
