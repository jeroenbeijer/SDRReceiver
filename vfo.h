#ifndef VFO_H
#define VFO_H

#include <qstring.h>
#include <QSharedPointer>
#include "zmqpublisher.h"
#include "dsp/halfbanddecimator.h"
#include "dsp/oscillator.h"
#include "complexsamplefanoutpool.h"
#include "audiobufferqueue.h"
#include "dsp/firhilbert.h"

class vfo : public QObject
{

    Q_OBJECT

public:

    ~vfo();
    vfo(QObject *parent = 0);

    void init(int samplesPerBuffer, int lateDecimate = 0);
    void process(ComplexSampleBuffer* buf);
    void setZmqAddress(QString bind);
    void setZmqTopic(QString topic);
    void setZmqTopicLSB(QString topic);
    void setQueue(AudioSampleBufferQueue* queue);

    QString getZmqTopic();
    void setFs(int samplerate);
    void setDecimationCount(int count);
    void setHalfbandTaps(int taps);
    void setMixerFreq(double freq);
    void setCenterFreq(double freq);
    double getCenterFreq();
    double getMixerFreq();
    int getOutRate();
    void setFilterBandwidth(double bw);
    void setGain(float g);
    void setDemodUSB(bool usb);
    bool getDemodUSB();
    void setVFOs(QVector<vfo*> *pVFOs);
    std::vector<cpx_typef> decimate[9];
    QVector<vfo*> * mpVFOs;

signals:

    void fftData(const QSharedPointer<std::vector<cpx_typef>> data);
    void bufferReady();

public slots:
    void fftVFOSlot(QString topic);

private:

    QString zmqAddress;
    QString zmqConnect;
    QString zmqTopic;
    QString zmqTopicLSB;

    int Fs;

    HalfBandDecimator *  hdecimator[8];

    FIR * fir_usb;
    FIR * fir_lsb;

    FIRHilbert * philbert;
    DelayThing<float>  delayT;

    FIR * fir_decI;
    FIR * fir_decQ;

    int decimateCount;
    int halfBandTaps;
    uint32_t outputRate;
    uint32_t samplesOut;

    Oscillator * pOsc_mix;



    float gain;

    double mixer_freq;
    double center_freq;

    void ssbDemod();
    void finalDecim();

    bool demodUSB;
    bool demodLSB;

    cpx_typef avecpt;

    int filterbw;

    bool laststageDecimate;
    int discard;

    bool emitFFT;

    int FFTcount;

    QSharedPointer<std::vector<cpx_typef>> sharedSamples;
    QSharedPointer<std::vector<short>> sharedTransmit;

    ComplexSampleFanoutPool bufferPool;
    AudioSampleBufferQueue* pAudioBufferQueue = nullptr;

};

#endif // VFO_H
