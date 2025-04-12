#pragma once

#include <type_traits>

template <typename T>
class Box {
public:
    // Helper function to create a Box
    template <typename... Args>
    static Box<T> New(Args&&... args) {
        return Box<T>(new T(args...));
    }

    explicit Box(T* ptr = nullptr) noexcept : mPtr(ptr) {}
    ~Box() { reset(); }
    Box(const Box&) = delete;
    Box(Box&& other) noexcept : mPtr(other.mPtr) { other.mPtr = nullptr; }

    // upcast constructor
    template <typename U>
        requires std::is_convertible_v<U*, T*>
    Box(Box<U>&& other) noexcept {
        reset(other.release());
    }

    Box& operator=(const Box&) = delete;
    Box& operator=(Box&& other) noexcept {
        if (this != &other) {
            mPtr = other.mPtr;
            other.mPtr = nullptr;
        }
        return *this;
    }

    // Converting move assignment for upcasting
    template <typename U>
        requires std::is_convertible_v<U*, T*>
    Box<T>& operator=(Box<U>&& other) noexcept {
        reset(other.release());
        return *this;
    }

    T* get() const noexcept { return mPtr; }
    T& operator*() const { return *mPtr; }
    T* operator->() const noexcept { return mPtr; }
    explicit operator bool() const noexcept { return mPtr != nullptr; }

    T* release() noexcept {
        T* temp = mPtr;
        mPtr = nullptr;
        return temp;
    }

    void reset(T* ptr = nullptr) noexcept {
        if (mPtr != nullptr) {
            delete mPtr;
        }
        mPtr = ptr;
    }

    // Swap with another Box
    void swap(Box& other) noexcept {
        T* temp_ptr = mPtr;
        mPtr = other.mPtr;
        other.mPtr = temp_ptr;
    }

private:
    T* mPtr = nullptr;
};

// Comparison operators
template <typename T1, typename T2>
bool operator==(const Box<T1>& lhs, const Box<T2>& rhs) noexcept {
    return lhs.get() == rhs.get();
}

template <typename T1, typename T2>
bool operator!=(const Box<T1>& lhs, const Box<T2>& rhs) noexcept {
    return !(lhs == rhs);
}

template <typename T>
bool operator==(const Box<T>& lhs, std::nullptr_t) noexcept {
    return !lhs;
}

template <typename T>
bool operator==(std::nullptr_t, const Box<T>& rhs) noexcept {
    return !rhs;
}

template <typename T>
bool operator!=(const Box<T>& lhs, std::nullptr_t) noexcept {
    return static_cast<bool>(lhs);
}

template <typename T>
bool operator!=(std::nullptr_t, const Box<T>& rhs) noexcept {
    return static_cast<bool>(rhs);
}

template <typename T, typename U>
bool operator==(const Box<T>& lhs, const U* rhs) noexcept {
    return lhs.get() == rhs;
}

template <typename T, typename U>
bool operator==(const U* lhs, const Box<T>& rhs) noexcept {
    return lhs == rhs.get();
}

template <typename T, typename U>
bool operator!=(const Box<T>& lhs, const U* rhs) noexcept {
    return lhs.get() != rhs;
}

template <typename T, typename U>
bool operator!=(const U* lhs, const Box<T>& rhs) noexcept {
    return lhs != rhs.get();
}
