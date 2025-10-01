#include <thread>
#include <QtConcurrent/QtConcurrent>
#include "rtlsdr.h"


RtlSdr::RtlSdr(QObject *parent) : Radio(parent)
{
    tcp_active = false;
    val = 0.000001;
    one = 1.0;
    tcpSocket = 0;
    worker = 0;

}
RtlSdr::~RtlSdr()
{

}

bool RtlSdr::StartRemote(QString device_address, int samplerate, int frequency, int gain)
{

    bool result = false;
    tcpSocket = new QTcpSocket(this);

    connect(tcpSocket, SIGNAL(connected()),this, SLOT(connected()));
    connect(tcpSocket, SIGNAL(disconnected()),this, SLOT(disconnected()));
    connect(tcpSocket, SIGNAL(bytesWritten(qint64)),this, SLOT(bytesWritten(qint64)));
    connect(tcpSocket, SIGNAL(readyRead()),this, SLOT(readyRead()));

    tcpFloats.resize((samplerate/4)*2);

    tcpData.resize(tcpFloats.size());

    tcpSocket->connectToHost(device_address.split(":")[0], device_address.split(":")[1].toInt());

    if(!tcpSocket->waitForConnected(5000))
    {
         qDebug() << "Error: " << tcpSocket->errorString();

    }
    else{

        uint agc_mode = 0;
        uint gain_mode = 1;


        sendCommand(CMD_SET_AGC_MODE, static_cast<char*>(static_cast<void*>(&agc_mode)));
        sendCommand(CMD_SET_TUNER_GAIN_MODE, static_cast<char*>(static_cast<void*>(&gain_mode)));
        sendCommand(CMD_SET_TUNER_GAIN_INDEX,static_cast<char*>(static_cast<void*>(&gain)));

        sendCommand(CMD_SET_SAMPLE_RATE, static_cast<char*>(static_cast<void*>(&samplerate)));
        sendCommand(CMD_SET_FREQ, static_cast<char*>(static_cast<void*>(&frequency)));

        result = true;

        tcp_active=true;
    }


    return result;
}

void RtlSdr::connected()
{
    qDebug() << "connected...";

}

void RtlSdr::disconnected()
{


}

void RtlSdr::bytesWritten(qint64 bytes)
{
    qDebug() << bytes << " bytes written...";
}

void RtlSdr::readyRead()
{
    // read the data from the socket
    qint64 avail = tcpSocket->bytesAvailable();

    if(avail == 12)
    {
        int _tunerType = 0;
        int _tunerGainCount = 0;

        QByteArray buffer = tcpSocket->readAll();

        if (buffer[0] != 'R' || buffer[1] != 'T' || buffer[2] != 'L' || buffer[3] != '0')
        {
             return;
        }
        _tunerType = (uint)(buffer[4] << 24 | buffer[5] << 16 | buffer[6] << 8 | buffer[7]);
        _tunerGainCount = (uint)(buffer[8] << 24 | buffer[9] << 16 | buffer[10] << 8 | buffer[11]);

        qDebug() << "Tuner type " << _tunerType << " gain count " << _tunerGainCount;

    }
    else if (avail >=tcpFloats.size() )
    {


        tcpData = tcpSocket->read(tcpFloats.size());
        unsigned char* buff = (unsigned char*)tcpData.data();
        for(int i=0;i<tcpData.size();++i)
        {

            tcpFloats[i]=floats.at(buff[i]);

        }

        demodData(tcpFloats.data(),tcpFloats.size());
    }
}

bool RtlSdr::sendCommand(char cmd, char *val)
{

    QByteArray cmdBuff;
    cmdBuff.resize(5);


    if (tcpSocket == 0 )
    {
         return false;
    }

    cmdBuff[0] = cmd;
    cmdBuff[1] = val[3];
    cmdBuff[2] = val[2];
    cmdBuff[3] = val[1];
    cmdBuff[4] = val[0];

    tcpSocket->write(cmdBuff);
    return true;
}

int RtlSdr::setCenterFreq(int freq)
{

    if(active)
    {
       return  rtlsdr_set_center_freq(rtldev, freq);
    }else
    {
       return -1;
    }
}

int RtlSdr::indexBySerial(const char *serial) {

    return(rtlsdr_get_index_by_serial(serial));

}

int RtlSdr::biasTee(int on, int device_idx)
{

    if(rtldev == NULL )
    {
        int open_res = rtlsdr_open(&rtldev,device_idx);

        if (open_res != 0)
        {
            return 0;
        }

        int t_res = rtlsdr_set_bias_tee(rtldev, on);

        rtlsdr_close(rtldev);
        rtldev = 0;

        if(t_res != 0)
        {
            return 0;
        }

    }else
    {

        int t_res = rtlsdr_set_bias_tee(rtldev, on);

        if(t_res != 0)
        {
            return 0;
        }

    }


    return 1;
}

QStringList RtlSdr::deviceNames()
{

    //fill Rtl device list
    QStringList device_names;
    int devcnt=rtlsdr_get_device_count();

    char manufact[256];
    char product[256];
    char serial[256];

    for(int i=0;i<devcnt;i++)
    {
        rtlsdr_get_device_usb_strings(i,
                                      (char*)&manufact,
                                      (char*)&product,
                                      (char*)&serial);

        device_names<<QString::fromLocal8Bit(rtlsdr_get_device_name(i))+ QString("-") + QString(serial);
    }


    return device_names;

}

void RtlSdr::StopAndCloseRtlTcp()
{
    if(tcp_active)
    {

        if(tcpSocket)
        {

            tcpSocket->disconnect();
            tcpSocket->disconnectFromHost();
            tcpSocket->deleteLater();
            tcpSocket = 0;

        }
    }

    tcp_active=false;

}

bool RtlSdr::Open(int device_idx)
{

    int open_res = rtlsdr_open(&rtldev,device_idx);
    if (open_res != 0)
    {
        return false;
    }

    return true;

}

void RtlSdr::Start(int samplerate, int frequency, int buflen, int gain)
{

    do_demod_dispatcher_cancel=false;
    active=false;
    buffers_head_ptr=0;
    buffers_tail_ptr=0;
    buffers_used=0;


    //reset buffer and go
    rtlsdr_reset_buffer(rtldev);
    future_demod_dispatcher = startDispatcher();

    rtlsdr_set_sample_rate(rtldev, samplerate);
    rtlsdr_set_center_freq(rtldev, frequency);
    rtlsdr_set_tuner_gain_mode(rtldev,1);
    rtlsdr_set_tuner_gain(rtldev, gain);
    rtlsdr_set_agc_mode(rtldev,0);

    worker = new RtlWorkerThread(buflen, rtldev, this);
    thread = new QThread;
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &RtlWorkerThread::start);

    connect(worker, &RtlWorkerThread::finished,
            this,
            [this]() {
                if (thread) {
                    thread->quit();
                    thread->wait();
                    thread->deleteLater();
                    thread = nullptr;
                }

                if (worker) {
                    worker->deleteLater();
                    worker = nullptr;
                }

                if (rtldev) {
                    rtlsdr_close(rtldev);
                    rtldev = nullptr;
                }
                active = false;
            });

    thread->start();

    active = true;

}

bool RtlSdr::StopAndClose()
{

    bool result = false;

    if(tcp_active){
        StopAndCloseRtlTcp();
        result = true;
    }


    else if(active)
    {

        result = StopRtl();

    }

    do_demod_dispatcher_cancel=false;
    active=false;
    buffers_head_ptr=0;
    buffers_tail_ptr=0;
    buffers_used=0;
    return result;
}

bool RtlSdr::StopRtl()
{

     if (!worker)
        return false;

    worker->stop();
    qDebug()<<"rtl sdr stopped";

    //stop demod_dispatcher
    if(!future_demod_dispatcher.isFinished())
    {
        qDebug()<<"stopping demod_dispatcher";
        buffers_mut.lock();
        do_demod_dispatcher_cancel=true;
        buffers_not_empty.wakeAll();
        buffers_mut.unlock();
        future_demod_dispatcher.waitForFinished();
        do_demod_dispatcher_cancel=false;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        qDebug()<<"demod_dispatcher stopped";
    }

    //clear return buffer state
    //qDebug()<<"clearing buffers";
    buffers_head_ptr=0;
    buffers_tail_ptr=0;
    buffers_used=0;
    for(int i=0;i<N_BUFFERS;i++)buffers[i].clear();

    return true;
}
float RtlSdr::adjustGain(int gain)
{
    return float(gain)/100;
}

float RtlSdr::getFFTGain()
{

    return 30.0f;
}
void RtlSdr::sdr_callback(unsigned char *buf, uint32_t len)
{

    //check if buffers have room for one more
    buffers_mut.lock();
    if(buffers_used>=N_BUFFERS)
    {
        qDebug()<<"Dropped RTL buffer!!";
        buffers_mut.unlock();
        return;
    }
    buffers_mut.unlock();

    //cycle buffers
    buffers_head_ptr%=N_BUFFERS;

    //resize buffer if needed
    buffers[buffers_head_ptr].resize(len);

    float *buffptr=buffers[buffers_head_ptr].data();


    for(uint32_t i=0;i<len;++i)
    {
        //convert to complex float
        float val =floats.at(buf[i]);

        *buffptr=val;
        buffptr++;
    }


    buffers_size_valid[buffers_head_ptr]=len;//save the actual buffer size to the user

    //advertise we have filled a buffer
    buffers_mut.lock();
    ++buffers_used;
    buffers_not_empty.wakeAll();
    buffers_mut.unlock();

    //goto next buffer
    buffers_head_ptr++;

    return;

}




