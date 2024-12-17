#pragma once

#include <vector>
namespace whal::stl {

// std::find from <algorithm> so I don't have to include the whole thing
template <class InputIterator, class T>
InputIterator find(InputIterator first, InputIterator last, const T& val) {
    while (first != last) {
        if (*first == val)
            return first;
        ++first;
    }
    return last;
}

template <class InputIt, class UnaryPred>
constexpr InputIt find_if(InputIt first, InputIt last, UnaryPred p) {
    for (; first != last; ++first)
        if (p(*first))
            return first;

    return last;
}

// a SIMPLE Map implementation that stores pairs in an array. Should be faster for N < 40, maybe more due to cache shit.
// requires default constructible V, comparable K
// get() is CONST so key should be IN THERE
template <typename K, typename V>
class Map {
    struct Pair {
        K key;
        V value;
    };

public:
    bool contains(const K& key) const { return _getIndex(key) != -1; }

    void insert(Pair pair) {
        const int ix = _getIndex(pair.key);
        if (ix != -1) {
            mPairs[ix].value = pair.value;
        } else {
            mPairs.push_back(Pair{
                .key = pair.key,
                .value = pair.value,
            });
        }
    }

    const V& get(const K& key) const {
        const int ix = _getIndex(key);
        if (ix == -1) {
            // return nonsense
            return mPairs[0].value;
        }
        return mPairs[ix].value;
    }

private:
    int _getIndex(const K& key) const {
        for (size_t i = 0; i < mPairs.size(); ++i) {
            if (mPairs[i].key == key) {
                return i;
            }
        }
        return -1;
    }
    std::vector<Pair> mPairs;
};

}  // namespace whal::stl
