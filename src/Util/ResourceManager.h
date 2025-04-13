#pragma once

#include <algorithm>
#include <cassert>
#include <fstream>
#include <vector>
#include "Util/FileUtils.h"
#include "Util/Memory/Arc.h"
#include "Util/String.h"
#include "Util/Types.h"

namespace whal {

// reads file resources of the same type, keeps N of them in memory, freeing the one that went the longest without a read.
template <typename T, s32 N>
class ResourceManager {
public:
    ResourceManager() = default;

    void clearCache() { mCache.clear(); }

    Arc<T> readData(const char* filePath) {
        for (auto it = mCache.begin(); it != mCache.end(); it++) {
            if (isEqualString(it->path, filePath)) {
                // found item in cache, move to back
                auto resource = it->data;
                std::rotate(it, it + 1, mCache.end());
                return resource;
            }
        }

        // if cache is full, remove oldest item
        if (mCache.size() >= N) {
            mCache.erase(mCache.begin());
        }

        assert(isExist(filePath) && "file path doesn't exist");

        std::ifstream file(filePath);
        Arc<T> newData = Arc<T>::New();
        file >> *newData;

        // use push_back so the ref count is 2
        mCache.push_back({filePath, newData});

        return newData;
    }

private:
    struct CacheItem {
        std::string path;
        Arc<T> data;
    };

    std::vector<CacheItem> mCache;
};

}  // namespace whal
