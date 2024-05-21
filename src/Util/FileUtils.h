#pragma once

#include <string>

#include "whalECS/src/Expected.h"

Expected<std::string> readFile(const char* filePath);
bool isExist(const char* filePath);
