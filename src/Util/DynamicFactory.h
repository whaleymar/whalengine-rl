#pragma once

#include <string_view>
#include <unordered_map>
#include "Factory.h"

namespace whal {

// This is a more flexible version of whal::Factory. Its entries can be modified at runtime.
// should only pass string literals to this class, so they have global static lifetimes
template <typename T>
class DynamicFactory {
public:
    inline DynamicFactory(const char* factoryName) : mFactoryName(factoryName) {}

    template <s32 N>
    inline DynamicFactory(std::string_view factoryName, const NameToCreator<T> (&entries)[N]) : mFactoryName(factoryName) {
        addEntries(entries);
    }

    void addEntry(std::string_view entryName, T creatorFunction) { mFactoryEntries.insert({entryName, creatorFunction}); }

    template <s32 N>
    void addEntries(const NameToCreator<T> (&entries)[N]) {
        for (size_t i = 0; i < N; i++) {
            NameToCreator<T> creator = entries[i];
            mFactoryEntries.insert({creator.mName, creator.mCreationFunction});
        }
    }

    void removeEntry(std::string_view entryName) { mFactoryEntries.erase(entryName); }

    bool getEntry(std::string_view entryName, T* creationPtr) const {
        if (mFactoryEntries.contains(entryName)) {
            *creationPtr = mFactoryEntries.at(entryName);
            return true;
        }
        return false;
    }

private:
    std::string_view mFactoryName;
    std::unordered_map<std::string_view, T> mFactoryEntries;
};

}  // namespace whal
