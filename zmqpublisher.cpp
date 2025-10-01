#include <iostream>
#include "zmqpublisher.h"
#include "qmessagebox.h"
#include "audiobufferpool.h"
#include "audiobufferpoolaccess.h"


ZmqPublisher::ZmqPublisher()
{

    bind = false;
    bindAddress = "tcp://*:6002";
    connected = false;
}


void ZmqPublisher::connect()
{

    if(!connected)
    {

        context = zmq_ctx_new();
        publisher = zmq_socket(context, ZMQ_PUB);

        int keepalive = 1;
        int keepalivecnt = 10;
        int keepaliveidle = 1;
        int keepaliveintrv = 1;
        int reconnectInterval = 1000;
        int maxReconnectInterval = 0;

        zmq_setsockopt(publisher, ZMQ_TCP_KEEPALIVE,(void*)&keepalive, sizeof(ZMQ_TCP_KEEPALIVE));
        zmq_setsockopt(publisher, ZMQ_TCP_KEEPALIVE_CNT,(void*)&keepalivecnt,sizeof(ZMQ_TCP_KEEPALIVE_CNT));
        zmq_setsockopt(publisher, ZMQ_TCP_KEEPALIVE_IDLE,(void*)&keepaliveidle,sizeof(ZMQ_TCP_KEEPALIVE_IDLE));
        zmq_setsockopt(publisher, ZMQ_TCP_KEEPALIVE_INTVL,(void*)&keepaliveintrv,sizeof(ZMQ_TCP_KEEPALIVE_INTVL));

        zmq_setsockopt(publisher, ZMQ_RECONNECT_IVL,(void*)&reconnectInterval,sizeof(ZMQ_RECONNECT_IVL));
        zmq_setsockopt(publisher, ZMQ_RECONNECT_IVL_MAX,(void*)&maxReconnectInterval,sizeof(ZMQ_RECONNECT_IVL_MAX));

        std::string connected_url = bindAddress.toUtf8().constData();

        if(bind)
        {
           zmqStatus = zmq_bind(publisher, connected_url.c_str() );

           if(zmqStatus <0)
           {

               int error = zmq_errno();

               QMessageBox msgBox;
               msgBox.setText("ZeroMQ could not bind to socket on provided URL " + bindAddress + " error code: " + QString::number(error) + "\r\nCheck if the URL and check if another SDR is already running");
               msgBox.exec();
           }
        }
        else
        {
             zmqStatus = zmq_connect(publisher, connected_url.c_str() );
        }

        // will set this to true regardless
        connected = true;
    }

}
void ZmqPublisher::setAddress(QString bind)
{
    bindAddress = bind;

}

void ZmqPublisher::setBind(bool b)
{

    bind = b;
}

void ZmqPublisher::bufferReady()
{

    QSharedPointer<AudioSampleBuffer> buffer;

    while (pAudioBufferQueue->tryPop(buffer)) {

        std::string topic_text = buffer->topic.toUtf8().constData();;
        uint32_t len         = buffer->len;
        uint32_t sampleRate  = buffer->sampleRate;

        unsigned char rate[4];

        memcpy(rate, &sampleRate, 4);

        if(len != 0)
        {

            zmq_send(publisher, topic_text.c_str(), 5, ZMQ_SNDMORE);
            zmq_send(publisher, rate, 4, ZMQ_SNDMORE );
            zmq_send(publisher, (unsigned char*)buffer->data.data(), len, 0 );
        }
        audioPool().release(buffer);
    }
}

void ZmqPublisher::setQueue(AudioSampleBufferQueue* queue)
{
    pAudioBufferQueue = queue;
}


