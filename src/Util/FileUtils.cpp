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

bool isExist(const char* filePath) {
    std::ifstream file(filePath);
    return file.is_open();
}
