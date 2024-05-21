#include "Util/FileUtils.h"

#include <format>
#include <fstream>
#include <sstream>

Expected<std::string> readFile(const char* filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return Expected<std::string>::error(std::format("FileNotFound: {}\n", filePath));
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool isExist(const char* filePath) {
    std::ifstream file(filePath);
    return file.is_open();
}
