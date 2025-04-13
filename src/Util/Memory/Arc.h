#pragma once

#include <atomic>
#include <type_traits>
#include "Util/Types.h"

namespace internal {
// Simple atomic operations for thread safety
// This is a minimal implementation without full STL

// class AtomicCount {
// public:
//     explicit AtomicCount(u64 v) noexcept : mCount(v) {}
//
//     u64 operator++() noexcept { return __sync_add_and_fetch(&mCount, 1); }
//
//     u64 operator--() noexcept { return __sync_sub_and_fetch(&mCount, 1); }
//
//     operator u64() const noexcept { return __sync_fetch_and_add(const_cast<volatile u64*>(&mCount), 0); }
//
// private:
//     // Using volatile for basic thread visibility
//     // Note: In a production environment, you'd use proper atomics
//     volatile u64 mCount;
// };

// Control block for reference counting
// TODO try to forward declare this and define it in a source file?
class IRefCnt {
public:
    // AtomicCount shared_count;  // Number of Rcs
    std::atomic<u64> shared_count;

    IRefCnt() noexcept : shared_count(1) {}
    virtual ~IRefCnt() = default;

    virtual void destroy() noexcept = 0;  // Destroy the managed object
};

// Control block with specific type information
template <typename T>
class RefCnt : public IRefCnt {
public:
    RefCnt(T* p) noexcept : mPtr(p) {}
    void destroy() noexcept override { delete mPtr; }

private:
    T* mPtr;
};

}  // namespace internal

template <typename T>
class Arc {
public:
    template <typename... Args>
    static Arc<T> New(Args&&... args) {
        return Arc<T>(new T(args...));
    }

    // Default constructor
    Arc() = default;

    // nullptr constructor
    Arc(std::nullptr_t) noexcept : mPtr(nullptr), mRefCounter(nullptr) {}

    // Constructor with raw pointer
    template <typename U>
    explicit Arc(U* ptr) {
        if (ptr) {
            mPtr = ptr;
            mRefCounter = new internal::RefCnt<U>(ptr);
        } else {
            mPtr = nullptr;
            mRefCounter = nullptr;
        }
    }

    // Copy constructor
    Arc(const Arc& other) noexcept : mPtr(other.mPtr), mRefCounter(other.mRefCounter) { increment_ref_count(); }

    // Converting copy constructor for upcasting
    template <typename U>
        requires std::is_convertible_v<U*, T*>
    Arc(const Arc<U>& other) noexcept : mPtr(other.mPtr), mRefCounter(other.mRefCounter) {
        increment_ref_count();
    }

    // Move constructor
    Arc(Arc&& other) noexcept : mPtr(other.mPtr), mRefCounter(other.mRefCounter) {
        other.mPtr = nullptr;
        other.mRefCounter = nullptr;
    }

    // Converting move constructor for upcasting
    template <typename U>
        requires std::is_convertible_v<U*, T*>
    Arc(Arc<U>&& other) noexcept : mPtr(other.mPtr), mRefCounter(other.mRefCounter) {
        other.mPtr = nullptr;
        other.mRefCounter = nullptr;
    }

    // Destructor
    ~Arc() { decrement_ref_count(); }

    // Copy assignment
    Arc& operator=(const Arc& other) noexcept {
        if (this != &other) {
            Arc temp(other);
            swap(temp);
        }
        return *this;
    }

    // Move assignment
    Arc& operator=(Arc&& other) noexcept {
        if (this != &other) {
            decrement_ref_count();
            mPtr = other.mPtr;
            mRefCounter = other.mRefCounter;
            other.mPtr = nullptr;
            other.mRefCounter = nullptr;
        }
        return *this;
    }

    // Reset to empty
    void reset() noexcept { Arc().swap(*this); }

    // Reset with new pointer
    template <typename U>
    void reset(U* ptr) {
        Arc(ptr).swap(*this);
    }

    // Reset with new pointer and deleter
    template <typename U, typename Deleter>
    void reset(U* ptr, Deleter deleter) {
        Arc(ptr, deleter).swap(*this);
    }

    // Swap with another Rc
    void swap(Arc& other) noexcept {
        T* temp_ptr = mPtr;
        internal::IRefCnt* temp_ctrl = mRefCounter;

        mPtr = other.mPtr;
        mRefCounter = other.mRefCounter;

        other.mPtr = temp_ptr;
        other.mRefCounter = temp_ctrl;
    }

    // Get the managed pointer
    T* get() const noexcept { return mPtr; }

    // Dereference operators
    T& operator*() const noexcept { return *mPtr; }

    T* operator->() const noexcept { return mPtr; }

    // Get reference count (for debugging/testing)
    u64 use_count() const noexcept { return mRefCounter ? static_cast<u64>(mRefCounter->shared_count) : 0; }

    // Check if this is the only reference
    bool unique() const noexcept { return use_count() == 1; }

    // Boolean conversion
    explicit operator bool() const noexcept { return mPtr != nullptr; }

private:
    T* mPtr = nullptr;
    internal::IRefCnt* mRefCounter = nullptr;

    template <typename U>
    friend class Arc;

    // for AtomicCount
    // void increment_ref_count() noexcept {
    //     if (mRefCounter) {
    //         ++mRefCounter->shared_count;
    //     }
    // }

    // for AtomicCount
    // void decrement_ref_count() noexcept {
    //     if (mRefCounter) {
    //         if (--mRefCounter->shared_count == 0) {
    //             mRefCounter->destroy();
    //             delete mRefCounter;
    //         }
    //     }
    // }

    void increment_ref_count() noexcept {
        if (mRefCounter) {
            mRefCounter->shared_count.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void decrement_ref_count() noexcept {
        if (mRefCounter) {
            // When decrementing the shared count, we need stronger ordering
            // to ensure proper visibility of all previous operations
            if (mRefCounter->shared_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                mRefCounter->destroy();
                delete mRefCounter;
                mRefCounter = nullptr;
            }
        }
    }
};

// Comparison operators
template <typename T1, typename T2>
bool operator==(const Arc<T1>& lhs, const Arc<T2>& rhs) noexcept {
    return lhs.get() == rhs.get();
}

template <typename T1, typename T2>
bool operator!=(const Arc<T1>& lhs, const Arc<T2>& rhs) noexcept {
    return !(lhs == rhs);
}

template <typename T>
bool operator==(const Arc<T>& lhs, std::nullptr_t) noexcept {
    return !lhs;
}

template <typename T>
bool operator==(std::nullptr_t, const Arc<T>& rhs) noexcept {
    return !rhs;
}

template <typename T>
bool operator!=(const Arc<T>& lhs, std::nullptr_t) noexcept {
    return static_cast<bool>(lhs);
}

template <typename T>
bool operator!=(std::nullptr_t, const Arc<T>& rhs) noexcept {
    return static_cast<bool>(rhs);
}

template <typename T, typename U>
bool operator==(const Arc<T>& lhs, const U* rhs) noexcept {
    return lhs.get() == rhs;
}

template <typename T, typename U>
bool operator==(const U* lhs, const Arc<T>& rhs) noexcept {
    return lhs == rhs.get();
}

template <typename T, typename U>
bool operator!=(const Arc<T>& lhs, const U* rhs) noexcept {
    return lhs.get() != rhs;
}

template <typename T, typename U>
bool operator!=(const U* lhs, const Arc<T>& rhs) noexcept {
    return lhs != rhs.get();
}
