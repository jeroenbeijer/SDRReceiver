#include "sdrj.h"


sdrj::sdrj(QObject *parent)
{


    tcp_active = false;
    count = 0;
    avecpt = 0;
    val = 0.000001;
    one = 1.0;
    correctDC = false;

    tcpSocket = 0;


}
sdrj::~sdrj()
{
    for(int a = 0; a<mpVFOs->length(); a++)
    {
        vfo * pvfo = mpVFOs->at(a);
        delete pvfo;
    }


}


bool sdrj::start_tcp_rtl(QString device_address, int samplerate, int frequency, int gain)
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


void sdrj::setVFOs(QVector<vfo*> * vfos)
{

    mpVFOs = vfos;

}

void sdrj::fftVFOSlot(QString topic)
{

    if(topic.compare("Main") ==0)
    {

        emitFFT = true;

        count = 0;
    }
    else
    {
        emitFFT = false;

        count = 0;
    }

}
void sdrj::setDCCorrection(bool correct)
{

    correctDC = correct;
}

void sdrj::connected()
{
    qDebug() << "connected...";

}

void sdrj::disconnected()
{


}

void sdrj::bytesWritten(qint64 bytes)
{
    qDebug() << bytes << " bytes written...";
}

void sdrj::readyRead()
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

bool sdrj::sendCommand(byte cmd, char *val)
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

int sdrj::setCenterFreq(int freq)
{

    if(active)
    {
       return  rtlsdr_set_center_freq(rtldev, freq);
    }else
    {
       return -1;
    }
}

int sdrj::biasTee(int on, int device_idx)
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


bool sdrj::StopAndCloseRtl()
{
    bool result=sdr::StopAndCloseRtl();

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


    return result;
}

void sdrj::demodData(const float* data,int len)
{

    samples.resize(len/2);

    for(int i=0;i<len/2;++i)
    {

        //convert to complex float
        cpx_typef curr = cpx_typef(data[2*i],data[2*i+1]);

        if(correctDC)
        {

            static cpx_typef avept=0;
            avept=avept*(1.0f-0.000001f)+0.000001f*curr;
            curr-=avept;
        }

        samples[i] = curr;
    }

    for(int a = 0; a<mpVFOs->length(); a++)
    {
        vfo * pvfo = mpVFOs->at(a);

        pvfo->process(samples);

    }

    int fftnt = 4;

    if(count == fftnt && emitFFT)
    {
        emit fftData(samples);
        count=0;
    }
    count++;

}
