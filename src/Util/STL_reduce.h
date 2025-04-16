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

template <class ForwardIt1, class ForwardIt2>
constexpr void iter_swap(ForwardIt1 a, ForwardIt2 b) {
    std::swap(*a, *b);
}

template <class ForwardIt>
constexpr ForwardIt rotate(ForwardIt first, ForwardIt middle, ForwardIt last) {
    if (first == middle)
        return last;

    if (middle == last)
        return first;

    ForwardIt write = first;
    ForwardIt next_read = first;  // read position for when “read” hits “last”

    for (ForwardIt read = middle; read != last; ++write, ++read) {
        if (write == next_read)
            next_read = read;  // track where “first” went
        stl::iter_swap(write, read);
    }

    // rotate the remaining sequence into place
    stl::rotate(write, next_read, last);
    return write;
}

template <class ForwardIt, class UnaryPred>
ForwardIt remove_if(ForwardIt first, ForwardIt last, UnaryPred p) {
    first = stl::find_if(first, last, p);
    if (first != last)
        for (ForwardIt i = first; ++i != last;)
            if (!p(*i))
                *first++ = std::move(*i);
    return first;
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

    void clear() { mPairs.clear(); }
    std::vector<Pair>& getData() { return mPairs; }

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
