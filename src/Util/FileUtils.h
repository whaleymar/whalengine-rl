#pragma once

#include <string>

#include "whalECS/src/Expected.h"

Expected<std::string> readFile(const char* filePath);
Expected<void> saveFile(const std::string& filePath, const std::string& data);
bool isExist(const char* filePath);
