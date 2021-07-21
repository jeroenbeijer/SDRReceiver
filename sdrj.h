#ifndef SDRJ_H
#define SDRJ_H

#include "jonti/sdr.h"
#include "vfo.h"

typedef std::complex<double> cpx_type;
typedef std::complex<float> cpx_typef;

const char CMD_SET_FREQ = 0x1;
const char CMD_SET_SAMPLE_RATE = 0x2;
const char CMD_SET_TUNER_GAIN_MODE = 0x3;
const char CMD_SET_GAIN = 0x4;
const char CMD_SET_FREQ_COR = 0x5;
const char CMD_SET_AGC_MODE = 0x8;
const char CMD_SET_TUNER_GAIN_INDEX = 0xd;

class sdrj : public sdr
{

Q_OBJECT

public:

    ~sdrj();
    explicit sdrj(QObject *parent = 0);

    void process_file(int sample_rate);
    void setDCCorrection(bool correct);
    int setCenterFreq(int freq);
    void setVFOs(QVector<vfo*> *pVFOs);
    int biasTee(int on, int device_idx);
    bool start_tcp_rtl(QString device_address, int sample_rate, int frequency, int gain = 496);
    bool StopAndCloseRtl();


signals:

    void fftData(const std::vector<cpx_typef> &data);

public slots:

   void demodData(const float*,int);
   void fftVFOSlot(QString topic);

   // socket stuff
   void connected();
   void disconnected();
   void bytesWritten(qint64 bytes);
   void readyRead();

private:

    bool sendCommand(byte cmd, char* val);
    bool tcp_active;
    std::vector<cpx_typef> samples;
    int count;
    QVector<vfo*> * mpVFOs;
    cpx_typef avecpt;

    float val;
    float one;

    bool emitFFT;
    bool correctDC;

    QTcpSocket *tcpSocket = nullptr;

    QByteArray tcpData;

    QVector<float> tcpFloats;


};

#endif // SDRJ_H
