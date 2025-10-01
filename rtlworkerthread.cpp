#include <thread>
#include "rtlworkerthread.h"
#include "rtlsdr.h"

void RtlWorkerThread::start()
{
    running.store(true);
    rtlsdr_read_async(dev, &RtlWorkerThread::rtlsdrCallback, this, 0, bufflen);
    emit finished();
}

void RtlWorkerThread::stop()
{

    running.store(false, std::memory_order_release);

    if (dev) {
        rtlsdr_cancel_async(dev);

    } else {
        qDebug() << "[worker] dev == nullptr in stop()";
    }
}

void RtlWorkerThread::rtlsdrCallback(unsigned char *buf, uint32_t len, void *ctx)
{
    RtlWorkerThread *self = static_cast<RtlWorkerThread*>(ctx);

    if (!self->running.load())
        return;

    self->rtl->sdr_callback(buf,len);

}
