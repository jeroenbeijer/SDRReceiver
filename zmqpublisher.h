#ifndef ZMQPUBLISHER_H
#define ZMQPUBLISHER_H

#include <QObject>
#include "QString"
#include "zmq.h"

class ZmqPublisher : public QObject
{
     Q_OBJECT

public:
    ZmqPublisher();

    void connect();
    void setAddress(QString address);
    void setBind(bool b = false);
    void publish(unsigned char *buf, uint32_t len, QString topic, uint32_t sampleRate);
    bool connected;


public slots:

    void transmitData(const QSharedPointer<std::vector<short>> data, uint32_t len, QString topic, uint32_t sampleRate );


private:

    void* context;
    void* publisher;
    QString bindAddress;
    int zmqStatus;
    bool bind;

};

#endif // ZMQPUBLISHER_H
