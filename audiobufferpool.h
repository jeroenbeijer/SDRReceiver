#pragma once

#include <QMutex>
#include <QSharedPointer>
#include <QString>

#include <vector>
#include <cstdint>
#include <complex>

// Generic reusable buffer
template<typename SampleT>
struct AudioBuffer
{
    std::vector<SampleT> data;
    QString topic;
    uint32_t len = 0;
    uint32_t sampleRate = 0;
};

template<typename SampleT>
class AudioBufferPool
{
public:
    using BufferType = AudioBuffer<SampleT>;

    QSharedPointer<BufferType> acquire();
    void release(QSharedPointer<BufferType> buffer);
    void reserve(size_t count);

private:
    QMutex m_mutex;
    std::vector<QSharedPointer<BufferType>> m_freeBuffers;
};

// ---- Public aliases (API) ----

using AudioSample = short;
using ComplexAudioSample    = std::complex<float>;

using AudioSampleBuffer = AudioBuffer<AudioSample>;
using ComplexAudioBuffer    = AudioBuffer<ComplexAudioSample>;

using AudioSampleBufferPool = AudioBufferPool<AudioSample>;
using ComplexAudioSampleBufferPool    = AudioBufferPool<ComplexAudioSample>;
