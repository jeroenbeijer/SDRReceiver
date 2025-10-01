#ifndef ZMQPUBLISHER_H
#define ZMQPUBLISHER_H

#include <QObject>
#include "QString"
#include "zmq.h"
#include "audiobufferqueue.h"


class ZmqPublisher : public QObject
{
     Q_OBJECT

public:
    ZmqPublisher();

    void connect();
    void setAddress(QString address);
    void setBind(bool b = false);
    bool connected;
    void setQueue(AudioSampleBufferQueue* queue);


public slots:

    void bufferReady();


private:

    void* context = nullptr;
    void* publisher = nullptr;
    QString bindAddress;
    int zmqStatus;
    bool bind;
    AudioSampleBufferQueue* pAudioBufferQueue = nullptr;

};

#endif // ZMQPUBLISHER_H
