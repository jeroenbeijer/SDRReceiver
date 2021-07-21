/* The MIT License (MIT)

Copyright (c) 2015-2019 Jonathan Olds

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#ifndef SDR_H
#define SDR_H

#include "qfuture.h"
#include <QVector>
#include "complex.h"
#include "jonti/fftrwrapper.h"
#include "qmutex.h"
#include "QWaitCondition"
#include <QTcpSocket>

extern "C" {
#include "rtl-sdr.h"
}

class sdr : public QObject
{

Q_OBJECT

public:

    sdr(QObject *parent = 0);
    bool OpenRtl(int device_idx);
    bool StopAndCloseRtl();
    QStringList deviceNames();
    void StartRtl(int sample_rate, int frequency, int gain = 496);

    ~sdr();
    rtlsdr_dev_t *rtldev;
    bool active;
    QVector<float> floats;


signals:

    void audio_signal_out(const float *inputBuffer, int size);


public slots:

private:

    bool StopRtl();


    //thread to send audio to demodulator
    bool demod_dispatcher();

    //thread to process the data from the RTL dongle
    void rtlsdr_callback(unsigned char *buf, uint32_t len);

    static void rtlsdr_callback_dispatcher(unsigned char *buf, uint32_t len, void *ctx)
    {
        sdr *inst=(sdr*)ctx;
        inst->rtlsdr_callback(buf,len);
    }

    //how we talk to the demod_dispatcher thread
    QFuture<bool> future_demod_dispatcher;
    bool do_demod_dispatcher_cancel;

    //how we talk to the rtl_callack thread
    QFuture<int> future_rtlsdr_callback;

    //For returning data from the rtl_callback thread to the demod_dispatcher thread
    #define N_BUFFERS 20

    QWaitCondition buffers_not_empty;
    QMutex buffers_mut;

    std::vector<float> buffers[N_BUFFERS];
    int buffers_size_valid[N_BUFFERS];

    int buffers_head_ptr=0;
    int buffers_tail_ptr=0;
    int buffers_used=0;
};


#endif // SDR_H
