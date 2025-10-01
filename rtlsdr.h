#ifndef RTLSDR_H
#define RTLSDR_H

#include <QTcpSocket>
#include "radio.h"
#include "rtlworkerthread.h"


extern "C" {
#include "rtl-sdr.h"
}

const char CMD_SET_FREQ = 0x1;
const char CMD_SET_SAMPLE_RATE = 0x2;
const char CMD_SET_TUNER_GAIN_MODE = 0x3;
const char CMD_SET_GAIN = 0x4;
const char CMD_SET_FREQ_COR = 0x5;
const char CMD_SET_AGC_MODE = 0x8;
const char CMD_SET_TUNER_GAIN_INDEX = 0xd;


class RtlSdr : public Radio
{

Q_OBJECT

public:

    ~RtlSdr();
    explicit RtlSdr(QObject *parent = 0);

    void process_file(int sample_rate);
    bool Open(int device_idx) override ;
    bool StartRemote(QString device_address,int sample_rate, int frequency,int gain = 496) override;
    void Start(int sample_rate, int frequency, int buflen, int gain = 496) override;
    bool StopAndClose() override;
    int indexBySerial(const char *serial) override;
    int setCenterFreq(int freq) override;
    int biasTee(int on, int device_idx) override;
    QStringList deviceNames() override ;
    float adjustGain(int gain) override;
    float getFFTGain() override;
    void sdr_callback(unsigned char *buf, uint32_t len);


public slots:


   // socket stuff
   void connected();
   void disconnected();
   void bytesWritten(qint64 bytes);
   void readyRead();

private:

    bool StopRtl();
    void StopAndCloseRtlTcp();
    bool sendCommand(char cmd, char* val);
    bool tcp_active;

    float val;
    float one;

    QTcpSocket *tcpSocket = nullptr;

    QByteArray tcpData;

    QVector<float> tcpFloats;

    rtlsdr_dev_t *rtldev;

    QObject * tParent;

    RtlWorkerThread *worker;
    QThread *thread = nullptr;


};

#endif // RTLSDR_H
