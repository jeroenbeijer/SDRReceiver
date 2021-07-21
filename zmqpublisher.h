#ifndef ZMQPUBLISHER_H
#define ZMQPUBLISHER_H

#include "QString"
#include "zmq.h"

class ZmqPublisher
{
public:
    ZmqPublisher();

    void connect();
    void setAddress(QString address);
    void setBind(bool b = false);
    void setTopic(QString topic);
    void publish(unsigned char *buf, uint32_t len, QString topic);
    bool connected;


private:

    void* context;
    void* publisher;
    QString bindAddress;
    int zmqStatus;
    bool bind;

};

#endif // ZMQPUBLISHER_H
