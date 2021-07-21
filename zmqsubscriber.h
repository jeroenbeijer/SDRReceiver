#ifndef ZMQSUBSCRIBER_H
#define ZMQSUBSCRIBER_H

#include <QObject>
#include <QThread>
#include <qbytearray.h>
#include <zmq.h>

class ZmqSubscriber : public QThread
{
    Q_OBJECT

public:

   ZmqSubscriber();

   void setBindAddress(QString address);
   void setBind(bool b = true);

   void run();



private:

   void* context;
   void* subscriber;

   int zmqStatus;

   QString bindAddress;

   bool bind;

signals:
    void recAudio(const QByteArray &audio, int recsize);
};

#endif // ZMQSUBSCRIBER_H
