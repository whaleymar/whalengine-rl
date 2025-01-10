#pragma once

#include <map>
#include <unordered_map>

namespace whal {

template <typename K, typename V>
std::map<K, V> sortMap(const std::unordered_map<K, V>& map) {
    std::map<K, V> result;
    for (const auto& [k, v] : map) {
        result.insert({k, v});
    }
    return result;
}

template <typename K, typename V>
std::unordered_map<K, V> unsortMap(const std::map<K, V>& map) {
    std::unordered_map<K, V> result;
    for (const auto& [k, v] : map) {
        result.insert({k, v});
    }
    return result;
}

}  // namespace whal
