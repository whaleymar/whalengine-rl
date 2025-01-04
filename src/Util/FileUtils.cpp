#include "Util/FileUtils.h"

#include <fstream>
#include "Util/Print.h"

Expected<std::string> readFile(const char* filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return Expected<std::string>::error(whal_format("FileNotFound: {}\n", filePath));
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

Expected<void> saveFile(const std::string& filePath, const std::string& data) {
    std::ofstream outFile(filePath, std::ios::out | std::ios::trunc);
    if (!outFile) {
        return Error(whal_format("Failed to open file: {}", filePath));
    }
    outFile << data;
    if (!outFile) {
        return Error(whal_format("Failed to write to file: {}", filePath));
    }
    return {};
}

bool isExist(const char* filePath) {
    std::ifstream file(filePath);
    return file.is_open();
}
