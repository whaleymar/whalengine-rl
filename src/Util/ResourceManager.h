#pragma once

#include <cassert>
#include <vector>
#include "Util/FileUtils.h"
#include "Util/IResource.h"
#include "Util/Memory/Arc.h"
#include "Util/STL_reduce.h"
#include "Util/String.h"
#include "Util/Types.h"

namespace whal {

// reads file resources of the same type, keeps N of them in memory, freeing the one that went the longest without a read.
template <typename T, s32 N>
    requires std::is_base_of_v<IResource, T>
class ResourceManager {
public:
    ResourceManager() = default;

    void clearCache() { mCache.clear(); }

    Arc<T> readData(const char* filePath) {
        for (auto it = mCache.begin(); it != mCache.end(); it++) {
            if (isEqualString(it->path, filePath)) {
                // found item in cache, move to back
                auto resource = it->data;
                stl::rotate(it, it + 1, mCache.end());
                return resource;
            }
        }

        // if cache is full, remove oldest item
        if (mCache.size() >= N) {
            mCache.erase(mCache.begin());
        }

        assert(isExist(filePath) && "file path doesn't exist");

        Arc<T> newData = Arc<T>::New();
        newData->load(filePath);

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
