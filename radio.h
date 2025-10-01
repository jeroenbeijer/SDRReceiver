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
SOFTWARE.

Leaving this permission notice in, portions of this code orginally came from Jonathan.

*/

#ifndef RADIO_H
#define RADIO_H

#include <QObject>
#include <QVector>
#include <qmutex.h>
#include <QWaitCondition>
#include <QtConcurrent/QtConcurrent>
#include <qfuture.h>
#include <complex.h>
#include <QSharedPointer>
#include "vfo.h"
#include "jonti/fftrwrapper.h"


typedef std::complex<double> cpx_type;
typedef std::complex<float> cpx_typef;

class radio : public QObject
{

    Q_OBJECT


public:

    ~radio();
    radio(QObject *parent = 0);

    void setVFOs(QVector<vfo*> *pVFOs);
    void setDCCorrection(bool correct);
    void setVFOThreads(bool threads);

    virtual QStringList deviceNames() = 0;
    virtual int indexBySerial(const char *serial) = 0;
    virtual int setCenterFreq(int freq) = 0;
    virtual int biasTee(int on, int device_idx) = 0;
    virtual bool Open(int device_idx) = 0;
    virtual bool StartRemote(QString device_address,int sample_rate, int frequency,int gain = 496) = 0;
    virtual void Start(int sample_rate, int frequency, int buflen, int gain = 496) = 0;
    virtual bool StopAndClose() = 0;
    virtual float adjustGain(int gain)=0;
    virtual float getFFTGain()=0;


signals:

    void fftData(const QSharedPointer<std::vector<cpx_typef>> data);
    void audio_signal_out(const float *inputBuffer, int size);
    void demodVFOData(const QSharedPointer<std::vector<cpx_typef>> data,int len);


public slots:

    void demodData(const float*,int);
    void fftVFOSlot(QString topic);

protected:

    QVector<vfo*> * mpVFOs;
    bool correctDC;

    //thread to send audio to demodulator
    bool demod_dispatcher();

    QFuture<bool> startDispatcher();

    //thread to process the data from the RTL dongle
    void sdr_callback(unsigned char *buf, uint32_t len);
    void sdr_callback(float *buf, uint32_t len);

    static void sdr_callback_dispatcher(unsigned char *buf, uint32_t len, void *ctx)
    {
        radio *inst=(radio*)ctx;
        inst->sdr_callback(buf,len);
    }

    //how we talk to the demod_dispatcher thread
    QFuture<bool> future_demod_dispatcher;
    bool do_demod_dispatcher_cancel;

    //how we talk to the rtl_callback thread
    QFuture<int> future_rtlsdr_callback;

    //For returning data from the rtl_callback thread to the demod_dispatcher thread
    #define N_BUFFERS 80

    QWaitCondition buffers_not_empty;
    QMutex buffers_mut;

    std::vector<float> buffers[N_BUFFERS];
    int buffers_size_valid[N_BUFFERS];

    int buffers_head_ptr=0;
    int buffers_tail_ptr=0;
    int buffers_used=0;

    bool active;
    QVector<float> floats;
    bool vfo_threads = true;


private:

   cpx_typef avecpt;
   int count;
   bool emitFFT;
   std::vector<cpx_typef> samples;
   QSharedPointer<std::vector<cpx_typef>> sharedSamples;
   cpx_typef avept = 0.0;



};

#endif // RADIO_H
