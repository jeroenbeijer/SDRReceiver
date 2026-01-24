#ifndef RTLWORKERTHREAD_H
#define RTLWORKERTHREAD_H

#include <QObject>

class RtlSdr;

extern "C" {
#include "rtl-sdr.h"
}

class RtlWorkerThread : public QObject
{
    Q_OBJECT
    public:
      explicit  RtlWorkerThread(int bufflen, rtlsdr_dev_t* dev, RtlSdr *rtl = nullptr, QObject *parent = nullptr )
            : QObject(parent), bufflen(bufflen), dev(dev), running(false), rtl(rtl) {}

    public slots:
        void start();
        void stop();

    signals:
        void finished();

    private:
        static void rtlsdrCallback(unsigned char *buf, uint32_t len, void *ctx);
        int bufflen;
        rtlsdr_dev_t* dev;
        std::atomic_bool running;
        RtlSdr * rtl;



};

#endif // RTLWORKERTHREAD_H
