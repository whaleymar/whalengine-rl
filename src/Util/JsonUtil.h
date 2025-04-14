// templated JSON code goes here so compile times aren't impacted for files that don't need this stuff

#pragma once

#include <vector>
#include "Util/JsonDoc.h"

namespace whal {

template <typename T>
T readVal(JsonValue json, const char* key) {
    // Integer types
    if constexpr (std::is_same_v<T, int> || std::is_same_v<T, long> || std::is_same_v<T, int32_t> || std::is_same_v<T, int64_t> ||
                  std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t> || std::is_same_v<T, unsigned int> || std::is_same_v<T, unsigned long> ||
                  std::is_enum_v<T>) {
        return static_cast<T>(json.getInt());
    }
    // Floating point types
    else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
        return static_cast<T>(json.getNumber());
    }
    // Boolean
    else if constexpr (std::is_same_v<T, bool>) {
        return json.getBool();
    }
    // String
    else if constexpr (std::is_same_v<T, std::string>) {
        return json.getString();
    }
    // Fall through for unsupported types
    else {
        // This will cause a compile error for unsupported types
        static_assert(std::is_void_v<T>, "Unsupported type for getValue<T>()");
        return 0;
    }
}

template <typename T>
bool tryReadVal(JsonValue data, const char* key, T* dst) {
    if (data.contains(key)) {
        *dst = readVal<T>(data[key], key);
        return true;
    }
    return false;
}

// Read a JSON array into a vector of integers
// Returns true if successful, false otherwise
template <typename T>
bool jsonArrayToVector(JsonValue json, std::vector<T>& outVector);

// Read a JSON property that contains an array into a vector of integers
template <typename T>
std::vector<T> jsonPropertyToVector(JsonValue json, const char* key) {
    if (!json.isObject() || !json.contains(key)) {
        return {};
    }

    std::vector<T> outVector;
    jsonArrayToVector<T>(json[key], outVector);
    return outVector;
}

}  // namespace whal
