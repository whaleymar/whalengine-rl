#pragma once

#include <string_view>
#include "DebugUtil.h"
#include "Util/Print.h"
#include "json.hpp"

namespace whal {

struct Color;

template <typename T>
T readVal(const nlohmann::json& data, std::string_view key) {
    DBG_ASSERT(data.contains(key), whal_format("Missing key: {}", key).c_str());
    return data[key];
}

template <typename T>
bool tryReadVal(const nlohmann::json& data, std::string_view key, T* dst) {
    if (data.contains(key)) {
        *dst = data[key];
        return true;
    }
    return false;
}

}  // namespace whal
