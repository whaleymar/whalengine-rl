#include "Util/JsonUtil.h"

namespace whal {

// Template specializations for common types
// Each specialization handles type-specific conversions

// Integer specialization
template <>
bool jsonArrayToVector<int>(JsonValue json, std::vector<int>& outVector) {
    if (!json.isArray()) {
        return false;
    }

    size_t size = json.size();
    outVector.clear();
    outVector.reserve(size);

    for (size_t i = 0; i < size; ++i) {
        outVector.push_back(static_cast<int>(json[i].getInt()));
    }

    return true;
}

// Float specialization - handles both int and float JSON values
template <>
bool jsonArrayToVector<float>(JsonValue json, std::vector<float>& outVector) {
    if (!json.isArray()) {
        return false;
    }

    size_t size = json.size();
    outVector.clear();
    outVector.reserve(size);

    for (size_t i = 0; i < size; ++i) {
        outVector.push_back(static_cast<float>(json[i].getNumber()));
    }

    return true;
}

// Double specialization - handles both int and float JSON values
template <>
bool jsonArrayToVector<double>(JsonValue json, std::vector<double>& outVector) {
    if (!json.isArray()) {
        return false;
    }

    size_t size = json.size();
    outVector.clear();
    outVector.reserve(size);

    for (size_t i = 0; i < size; ++i) {
        outVector.push_back(json[i].getNumber());
    }

    return true;
}

// String specialization
template <>
bool jsonArrayToVector<std::string>(JsonValue json, std::vector<std::string>& outVector) {
    if (!json.isArray()) {
        return false;
    }

    size_t size = json.size();
    outVector.clear();
    outVector.reserve(size);

    for (size_t i = 0; i < size; ++i) {
        outVector.push_back(json[i].getString());
    }

    return true;
}

// Boolean specialization
template <>
bool jsonArrayToVector<bool>(JsonValue json, std::vector<bool>& outVector) {
    if (!json.isArray()) {
        return false;
    }

    size_t size = json.size();
    outVector.clear();
    // Note: std::vector<bool> doesn't have normal reserve semantics

    for (size_t i = 0; i < size; ++i) {
        outVector.push_back(json[i].getBool());
    }

    return true;
}

}  // namespace whal
