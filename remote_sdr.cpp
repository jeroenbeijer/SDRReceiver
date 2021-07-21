#include "remote_sdr.h"
#include <qstringlist.h>
#include <qstring.h>
#include <QtConcurrent/QtConcurrent>
#include <iostream>
#include "kiss_fft130/kiss_fftr.h"
#include "zmq_utils.h"
#include "zmq.h"
#include "qvector.h"

remote_sdr::remote_sdr(QObject *parent, int Fs) : QObject(parent)
{



   sdr_active = false;
   file_active = false;
}

QStringList remote_sdr::deviceNames()
{

     //fill Rtl device list
    QStringList device_names;
    int devcnt=rtlsdr_get_device_count();
    for(int i=0;i<devcnt;i++)
    {
        device_names<<QString::fromLocal8Bit(rtlsdr_get_device_name(i));
    }


    return device_names;

}

void remote_sdr::setRemote(boolean rem)
{
    remote = rem;
}

void remote_sdr::setBindAddress(QString address)
{
    bindAddress = address;
}

bool remote_sdr::open_rtl(int device_idx)
{

    rtlsdr_open(&rtldev,device_idx);
    if (NULL == rtldev)
    {
        std::cout << "Could not open device " << std::flush;
        return false;
    }


    return true;

}

void remote_sdr::start_rtl(int samplerate, int frequency)
{

    if(!sdr_active)
    {
        future_demod_dispatcher = QtConcurrent::run(this,&remote_sdr::demod_dispatcher);

        //reset buffer and go
        rtlsdr_reset_buffer(rtldev);

        rtlsdr_set_sample_rate(rtldev, samplerate);
        rtlsdr_set_center_freq(rtldev, frequency);


        rtlsdr_set_tuner_gain_mode(rtldev,1);
        rtlsdr_set_tuner_gain(rtldev, 496);

        //future_rtlsdr_callback = QtConcurrent::run(rtlsdr_read_async, rtldev,(rtlsdr_read_async_cb_t)rtlsdr_callback_dispatcher, this,0,int((2*samplerate)/4));//16384*2);

        future_rtlsdr_callback = QtConcurrent::run(rtlsdr_read_async, rtldev,(rtlsdr_read_async_cb_t)rtlsdr_callback_dispatcher, this,0,int(16384*10));

        sdr_active = true;
    }

}

void remote_sdr::rtlsdr_callback(unsigned char *buf, uint32_t len)
{
    if(sdr_active || file_active )
    {

            for(uint32_t i=0;i<len/2;++i)
            {

                //convert to complex doubles
                cpx_type curr = cpx_type(((double)((qint16)buf[2*i] - 127)),((double)((qint16)buf[2*i+1] - 127)));

                 fft[i] = curr;
             }

            if(counter == 1)
            {

                 emit fftData(fft);

                 counter = 0;
            }else
            {
                counter++;
             }


            QByteArray qdata((const char*)buf, len);
            publish(qdata);

    }


}

bool remote_sdr::demod_dispatcher()
{

    return do_demod_dispatcher_cancel;

}


void remote_sdr::connectZMQ()
{
    // bind socket

    context = zmq_ctx_new();
    publisher = zmq_socket(context, ZMQ_PUB);

    int keepalive = 1;
    int keepalivecnt = 10;
    int keepaliveidle = 1;
    int keepaliveintrv = 1;
    int reconnectInterval = 1000;
    int maxReconnectInterval = 0;
    int sendBufBytes = 6384000;

    QString topic = "AUDIO";
    std::string topic_std = topic.toUtf8().constData();

    zmq_setsockopt(publisher, ZMQ_IDENTITY, topic_std.c_str(), 5);
    zmq_setsockopt(publisher, ZMQ_TCP_KEEPALIVE,(void*)&keepalive, sizeof(ZMQ_TCP_KEEPALIVE));
    zmq_setsockopt(publisher, ZMQ_TCP_KEEPALIVE_CNT,(void*)&keepalivecnt,sizeof(ZMQ_TCP_KEEPALIVE_CNT));
    zmq_setsockopt(publisher, ZMQ_TCP_KEEPALIVE_IDLE,(void*)&keepaliveidle,sizeof(ZMQ_TCP_KEEPALIVE_IDLE));
    zmq_setsockopt(publisher, ZMQ_TCP_KEEPALIVE_INTVL,(void*)&keepaliveintrv,sizeof(ZMQ_TCP_KEEPALIVE_INTVL));

   // zmq_setsockopt(publisher, ZMQ_RECONNECT_IVL,(void*)&reconnectInterval,sizeof(ZMQ_RECONNECT_IVL));
   // zmq_setsockopt(publisher, ZMQ_RECONNECT_IVL_MAX,(void*)&maxReconnectInterval,sizeof(ZMQ_RECONNECT_IVL_MAX));
    zmq_setsockopt(publisher, ZMQ_SNDBUF,(void*)&sendBufBytes,sizeof(ZMQ_SNDBUF));



    std::string connected_url = bindAddress.toUtf8().constData();

    int zmqStatus= zmq_connect(publisher, connected_url.c_str() );

    int error = zmq_errno();

    std::cout << "zmq status " << zmqStatus << "error no " << error << std::flush << "\r\n";

}

void remote_sdr::publish(unsigned char *buf, uint32_t len){

    QString topic = "AUDIO";
    std::string topic_text = topic.toUtf8().constData();

    if(len != 0){

        zmq_send(publisher, topic_text.c_str(), 5, ZMQ_SNDMORE);
        zmq_send(publisher, buf, len, 0 );
    }

}
void remote_sdr::publish(QByteArray data){

    QString topic = "AUDIO";
    std::string topic_text = topic.toUtf8().constData();

    if(data.length() != 0){

        zmq_send(publisher, topic_text.c_str(), 5, ZMQ_SNDMORE);
        zmq_send(publisher, data.data(), data.length(), 0 );
    }

}
bool remote_sdr::close_rtl()
{

    if(sdr_active)
    {
       rtlsdr_cancel_async(rtldev);

       rtlsdr_close(rtldev);
    }
    sdr_active = false;


    return true;

}


void remote_sdr::init(int sample_rate)
{


    counter = 0;

    remote = true;

    sdr_active = false;

    fft.resize(16384*5);

    connectZMQ();

}

void remote_sdr::process_file(int samplerate, int frequency)
{



    QString filename = "C:/temp/IQ_rec_288000.bin";
    pInFile = new QFile(filename);
    pInFile->open(QIODevice::ReadOnly);


    QDataStream in(pInFile);


    int framelenbytes = 16384 * 10;
    unsigned char data[16384 * 10];


    int len = pInFile->bytesAvailable();
    int loop = len/framelenbytes;

    file_active = true;

    for(int z = 0; z<loop; z++)
    {

        qint64 lineLength = in.readRawData((char*)&data, framelenbytes);
        rtlsdr_callback((unsigned char*)&data, framelenbytes);

        Sleep(250);


    }

    file_active = false;
    pInFile->close();


}


