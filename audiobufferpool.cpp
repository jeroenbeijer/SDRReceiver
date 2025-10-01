#include "audiobufferpool.h"

#include <QMutexLocker>



template<typename SampleT>
QSharedPointer<typename AudioBufferPool<SampleT>::BufferType>
AudioBufferPool<SampleT>::acquire()
{
    QMutexLocker lock(&m_mutex);

    if (!m_freeBuffers.empty()) {
        auto buf = m_freeBuffers.back();
        m_freeBuffers.pop_back();
        return buf;
    }

    return QSharedPointer<BufferType>::create();
}

template<typename SampleT>
void AudioBufferPool<SampleT>::release(QSharedPointer<BufferType> buffer)
{
    QMutexLocker lock(&m_mutex);
    m_freeBuffers.push_back(std::move(buffer));
}

template<typename SampleT>
void AudioBufferPool<SampleT>::reserve(size_t count)
{
    QMutexLocker lock(&m_mutex);
    while (m_freeBuffers.size() < count) {
        m_freeBuffers.push_back(QSharedPointer<BufferType>::create());
    }
}

// ---- Explicit instantiations ----

template class AudioBufferPool<AudioSample>;
template class AudioBufferPool<ComplexAudioSample>;
