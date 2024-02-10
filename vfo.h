#ifndef VFO_H
#define VFO_H

#include "qstring.h"
#include "zmqpublisher.h"
#include "halfbanddecimator.h"
#include "oscillator.h"



class vfo : public QObject
{

    Q_OBJECT

public:

    ~vfo();
    vfo(QObject *parent = 0);

    void init(int samplesPerBuffer, bool bind, int lateDecimate = 0);
    void process(const std::vector<cpx_typef> & samples);
    void setZmqAddress(QString bind);
    void setZmqTopic(QString topic);
    void setZmqTopicLSB(QString topic);

    QString getZmqTopic();
    void setScaleComp(int scale);
    void setFs(int samplerate);
    void setDecimationCount(int count);
    void setHalfbandTaps(int taps);
    void setMixerFreq(double freq);
    void setCenterFreq(double freq);
    double getCenterFreq();
    double getMixerFreq();
    int getOutRate();
    void setOffsetBandwidth(double bw);
    void setFilterBandwidth(double bw);
    void setGain(float g);
    void setDemodUSB(bool usb);
    bool getDemodUSB();
    void setCompressonStyle(int st);
    void setFilter(bool filter, int bw = 0);
    void setVFOs(QVector<vfo*> *pVFOs);
    std::vector<cpx_typef> decimate[9];
    QVector<vfo*> * mpVFOs;



signals:

    void fftData(const std::vector<cpx_typef> &data);

public slots:
     void fftVFOSlot(QString topic);

private:

    QString zmqAddress;
    QString zmqConnect;
    QString zmqTopic;
    QString zmqTopicLSB;

    int Fs;
    bool zmqBind;

    //static publisher when binding
    static ZmqPublisher bind_publisher;
    ZmqPublisher connect_publisher;

    HalfBandDecimator *  hdecimator[8];


    QVector<cpx_typef> out;

    std::vector<short> transmit_usb;
    std::vector<short> transmit_lsb;
    std::vector<signed char> transmit_iq;


    FIR * fir_usb;
    FIR * fir_lsb;

    FIRHilbert * philbert;
    DelayThing<float>  delayT;

    FIR * fir_decI;
    FIR * fir_decQ;

    int decimateCount;
    int halfBandTaps;
    uint32_t outputRate;

    //WaveTable * mix_bfo;
    Oscillator * osc_bfo;
    Oscillator * osc_mix;

    float gain;

    double mixer_freq;
    double center_freq;

    void usb_demod();
    void usb_decimdemod();
    void compress();
    void transmitData();

    bool demodUSB;
    bool demodLSB;
    bool filterAudio;

    int cstyle = 0;

    cpx_typef avecpt;
    float val;
    float one;

    int filterbw;
    int offsetbw;

    bool laststageDecimate;
    int discard;

    bool emitFFT;

    int FFTcount;

    int scalecomp;

};

#endif // VFO_H
