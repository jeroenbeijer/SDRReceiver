#ifndef SDRPLAY_H
#define SDRPLAY_H

#include "radio.h"
#include "sdrplay_api.h"
#include <qfuture.h>

class sdrplay : public radio
{
public:
    explicit sdrplay(QObject *parent = nullptr);

    void Start(int sample_rate, int frequency, int buflen, int gain = 496) override;
    bool Open(int device_idx) override ;
    bool StartRemote(QString device_address,int sample_rate, int frequency,int gain = 496) override;
    bool StopAndClose() override;
    int indexBySerial(const char *serial) override;
    int setCenterFreq(int freq) override;
    int biasTee(int on, int device_idx) override;
    QStringList deviceNames() override ;
    void processCallback(short *xi, short *xq, unsigned int numSamples);
    float adjustGain(int gain) override;
    float getFFTGain() override;


private:

    sdrplay_api_DeviceT devs[6];

    sdrplay_api_DeviceT *chosenDevice = NULL;

    sdrplay_api_ErrT err;

    sdrplay_api_DeviceParamsT *deviceParams = NULL;

    sdrplay_api_CallbackFnsT cbFns;

    sdrplay_api_RxChannelParamsT *chParams;

    unsigned int ndev = 0;

    QFuture<sdrplay_api_ErrT> future_sdrplay_callback;

    std::vector<float> buffer;

    unsigned int bufferPtr = 0;

    bool running = false;

    int biasT = 0;

    const float scale = 1.0f / 32768.0f;

};

#endif // SDRPLAY_H
