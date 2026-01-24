#pragma once

#include <QMutex>
#include <QSharedPointer>
#include <deque>

#include "audiobufferpool.h"

// Generic buffer queue
template<typename SampleT>
class AudioBufferQueue
{
public:
    using BufferType = AudioBuffer<SampleT>;

    void push(QSharedPointer<BufferType> buffer);
    bool tryPop(QSharedPointer<BufferType>& out);

private:
    QMutex m_mutex;
    std::deque<QSharedPointer<BufferType>> m_queue;
};

// ---- Concrete aliases ----

using AudioSampleBufferQueue = AudioBufferQueue<AudioSample>;
using ComplexAudioSampleBufferQueue    = AudioBufferQueue<ComplexAudioSample>;


