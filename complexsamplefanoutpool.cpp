#include "complexsamplefanoutpool.h"
#include <thread>

ComplexSampleFanoutPool::ComplexSampleFanoutPool(std::size_t poolSize)
    : poolSize_(poolSize),
    pool_(poolSize)
{
    for (auto& buf : pool_) {
        buf.remaining.store(0, std::memory_order_relaxed);
    }
}

ComplexSampleBuffer* ComplexSampleFanoutPool::acquireBuffer(int consumerCount)
{
    for (;;) {
        std::size_t idx =
            writeIndex_.load(std::memory_order_relaxed);

        ComplexSampleBuffer& buf = pool_[idx];

        // Is this buffer free?
        if (buf.remaining.load(std::memory_order_acquire) == 0) {

            // Advance write index
            writeIndex_.store((idx + 1) % poolSize_,
                              std::memory_order_relaxed);

            // Publish: set refcount
            buf.remaining.store(consumerCount,
                                std::memory_order_release);

            return &buf;
        }

        // All buffers busy — back off
        std::this_thread::yield();
    }
}

