#ifndef COMPLEXSAMPLEFANOUTPOOL_H
#define COMPLEXSAMPLEFANOUTPOOL_H

#include <vector>
#include <atomic>
#include <complex>
#include <cstddef>

using cpx_typef = std::complex<float>;

struct ComplexSampleBuffer
{
    std::vector<cpx_typef> samples;
    std::atomic<int> remaining{0};
};

class ComplexSampleFanoutPool
{
public:
    explicit ComplexSampleFanoutPool(std::size_t poolSize);

    ComplexSampleBuffer* acquireBuffer(int consumerCount);

    std::size_t size() const { return poolSize_; }

private:
    const std::size_t poolSize_;
    std::vector<ComplexSampleBuffer> pool_;
    std::atomic<std::size_t> writeIndex_{0};
};

#endif // COMPLEXSAMPLEFANOUTPOOL_H

