#pragma once

#include <algorithm>
#include <fstream>
#include <vector>
#include "Util/String.h"

namespace whal {

// reads file resources of the same type, keeps N of them in memory, freeing the one that went the longest without a read.
template <typename T, s32 N>
class ResourceManager {
public:
    ResourceManager() = default;

    void clearCache() { mCache.clear(); }

    const T& readData(const char* filePath) {
        for (auto it = mCache.begin(); it != mCache.end(); it++) {
            if (isEqualString(it->path, filePath)) {
                // found item in cache, move to back
                std::rotate(it, it + 1, mCache.end());
                return mCache.back().data;
            }
        }

        // if cache is full, remove oldest item
        if (mCache.size() >= N) {
            mCache.erase(mCache.begin());
        }

        std::ifstream file(filePath);
        T newData;
        file >> newData;

        mCache.push_back({filePath, std::move(newData)});

        return mCache.back().data;
    }

private:
    struct CacheItem {
        std::string path;
        T data;
    };

    std::vector<CacheItem> mCache;
};

}  // namespace whal
