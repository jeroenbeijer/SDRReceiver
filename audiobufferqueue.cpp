#include "audiobufferqueue.h"

#include <QMutexLocker>

// ---- Template method definitions ----

template<typename SampleT>
void AudioBufferQueue<SampleT>::push(QSharedPointer<BufferType> buffer)
{
    QMutexLocker lock(&m_mutex);
    m_queue.push_back(std::move(buffer));
}

template<typename SampleT>
bool AudioBufferQueue<SampleT>::tryPop(QSharedPointer<BufferType>& out)
{
    QMutexLocker lock(&m_mutex);

    if (m_queue.empty())
        return false;

    out = std::move(m_queue.front());
    m_queue.pop_front();
    return true;
}

// ---- Explicit instantiations ----

template class AudioBufferQueue<AudioSample>;
template class AudioBufferQueue<ComplexAudioSample>;
