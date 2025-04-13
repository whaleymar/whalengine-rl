#include "Arc.h"

#include <atomic>

namespace internal {

class RefCntImpl {
public:
    RefCntImpl() : mCount(1) {}

    void addRef() noexcept { mCount.fetch_add(1, std::memory_order_relaxed); }

    bool release() noexcept { return mCount.fetch_sub(1, std::memory_order_acq_rel) == 1; }

    u64 getCount() const noexcept { return mCount.load(std::memory_order_relaxed); }

private:
    std::atomic<u64> mCount;
};

IRefCnt::IRefCnt() noexcept : mImpl(new RefCntImpl()) {}

IRefCnt::~IRefCnt() {
    delete mImpl;
}

void IRefCnt::addRef() noexcept {
    mImpl->addRef();
}

bool IRefCnt::release() noexcept {
    return mImpl->release();
}

u64 IRefCnt::getCount() const noexcept {
    return mImpl->getCount();
}

}  // namespace internal
