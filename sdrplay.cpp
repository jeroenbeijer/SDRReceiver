#include "sdrplay.h"
#include <thread>
#include <iostream>
#include "qmessagebox.h"


void StreamACallback (short *xi, short *xq, sdrplay_api_StreamCbParamsT *params, unsigned int numSamples,unsigned int reset, void *cbContext)
{

    Q_UNUSED(params);
    Q_UNUSED(reset);

    sdrplay *sdrp = (sdrplay*)cbContext;

    sdrp->processCallback(xi,xq,numSamples);

}

// not supported
void StreamBCallback (short *xi, short *xq,sdrplay_api_StreamCbParamsT *params,unsigned int numSamples, unsigned int reset, void *cbContext) {

    Q_UNUSED(xi);
    Q_UNUSED(xq);
    Q_UNUSED(params);
    Q_UNUSED(numSamples);
    Q_UNUSED(cbContext);
    Q_UNUSED(reset);
}

void EventCallback (sdrplay_api_EventT eventId, sdrplay_api_TunerSelectT tuner,sdrplay_api_EventParamsT *params, void *cbContext) {

    Q_UNUSED(eventId);
    Q_UNUSED(tuner);
    Q_UNUSED(params);
    Q_UNUSED(cbContext);

}


sdrplay::sdrplay(QObject *parent)
    : radio{parent}
{}

void sdrplay::Start(int sample_rate, int frequency, int buflen, int gain)
{

    future_demod_dispatcher = startDispatcher();

    buffer.resize(buflen);

    deviceParams->devParams->fsFreq.fsHz = (float)sample_rate;
    chParams = deviceParams->rxChannelA;

    chParams->tunerParams.rfFreq.rfHz = double(frequency);


    switch(chosenDevice->hwVer)
    {

        case SDRPLAY_RSP1A_ID :
            chParams->rsp1aTunerParams.biasTEnable = biasT;
            break;

        case SDRPLAY_RSP2_ID :
            chParams->rsp2TunerParams.biasTEnable = biasT;
            break;

        case SDRPLAY_RSPduo_ID :
            chParams->rspDuoTunerParams.biasTEnable = biasT;
            break;

        case SDRPLAY_RSPdx_ID :
            deviceParams->devParams->rspDxParams.biasTEnable = biasT;
            break;

        case SDRPLAY_RSP1B_ID :
            chParams->rsp1aTunerParams.biasTEnable = biasT;
            break;

        case SDRPLAY_RSPdxR2_ID :
            deviceParams->devParams->rspDxParams.biasTEnable = biasT;
            break;

    }

    //chParams->tunerParams.bwType = sdrplay_api_BW_1_536;
    chParams->tunerParams.bwType = sdrplay_api_BW_8_000;
    chParams->tunerParams.ifType = sdrplay_api_IF_Zero;
    chParams->tunerParams.gain.gRdB = gain;
    chParams->tunerParams.gain.LNAstate = 0;
    chParams->ctrlParams.agc.enable = sdrplay_api_AGC_DISABLE;

    cbFns.StreamACbFn = StreamACallback;
    cbFns.StreamBCbFn = StreamBCallback;
    cbFns.EventCbFn = EventCallback;

    future_sdrplay_callback = QtConcurrent::run(sdrplay_api_Init, chosenDevice->dev,&cbFns,this);

    err = future_sdrplay_callback.result();

    if(err == 0)
    {
        running = true;
    } else
    {

        QMessageBox msgBox;
        msgBox.setText("Unable to start sdrplay, check settings in ini file are correct, error code: " + QString::number(err));
        msgBox.exec();

    }



}

bool sdrplay::Open(int device_idx)
{

    if ((err = sdrplay_api_Open()) != sdrplay_api_Success)
    {
        printf("sdrplay_api_Open failed %s\n", sdrplay_api_GetErrorString(err));
    }

    //err = sdrplay_api_DisableHeartbeat();

    chosenDevice = &devs[device_idx];

    chosenDevice->tuner = sdrplay_api_Tuner_A;

    if ((err = sdrplay_api_SelectDevice(chosenDevice)) != sdrplay_api_Success)
    {
        printf("sdrplay_api_SelectDevice failed %s\n", sdrplay_api_GetErrorString(err));
        sdrplay_api_UnlockDeviceApi();
    }
    // Unlock API now that device is selected
    sdrplay_api_UnlockDeviceApi();

    // Retrieve device parameters so they can be changed if wanted
    if ((err = sdrplay_api_GetDeviceParams(chosenDevice->dev, &deviceParams)) !=
        sdrplay_api_Success)
    {
        printf("sdrplay_api_GetDeviceParams failed %s\n",
               sdrplay_api_GetErrorString(err));
        sdrplay_api_Close();

    }

    if (err != 0)
    {

        QMessageBox msgBox;
        msgBox.setText("Unable to open sdrplay, error code: " + QString::number(err));
        msgBox.exec();

    }

    return true;
}

// empty placeholder, remote not supported on sdrplay
bool sdrplay::StartRemote(QString device_address, int sample_rate, int frequency, int gain)
{

    Q_UNUSED(device_address);
    Q_UNUSED(sample_rate);
    Q_UNUSED(frequency);
    Q_UNUSED(gain);

    return false;
}

bool sdrplay::StopAndClose()
{

    int result=1;

    if(running){

        err = sdrplay_api_Uninit(chosenDevice->dev);

        sdrplay_api_ReleaseDevice(chosenDevice);

        sdrplay_api_UnlockDeviceApi();

        sdrplay_api_Close();

        if(!future_sdrplay_callback.isFinished())
        {
            qDebug()<<"stopping sdrplay_callback";

            future_rtlsdr_callback.waitForFinished();
            result=future_rtlsdr_callback.result();
            if(result)qDebug()<<"Error stopping thread";
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));//100ms seems to be the min
            qDebug()<<"sdrplay_callback stopped";
        }

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
    }
    //clear return buffer state
    //qDebug()<<"clearing buffers";
    buffers_head_ptr=0;
    buffers_tail_ptr=0;
    buffers_used=0;
    for(int i=0;i<N_BUFFERS;i++)buffers[i].clear();

    running = false;

    if(result)return false;
    return true;
}

int sdrplay::indexBySerial(const char *serial)
{
    int result = -2;


    if ((err = sdrplay_api_Open()) != sdrplay_api_Success)
    {
        printf("sdrplay_api_Open failed %s\n", sdrplay_api_GetErrorString(err));
    }


    sdrplay_api_LockDeviceApi();

    // Fetch list of available devices
    if ((err = sdrplay_api_GetDevices(devs, &ndev, sizeof(devs) /
                                                       sizeof(sdrplay_api_DeviceT))) != sdrplay_api_Success)
    {
        printf("sdrplay_api_GetDevices failed %s\n", sdrplay_api_GetErrorString(err));

    }

    else
    {

        QString device = "";

        for(unsigned int a = 0; a < ndev; a++)
        {

            if( QString(devs[a].SerNo) == QString(serial))
            {
                result = a;
                break;
            }
        }
    }

    sdrplay_api_UnlockDeviceApi();

    sdrplay_api_Close();

    return result;
}

int sdrplay::setCenterFreq(int freq)
{
    Q_UNUSED(freq);

    // Not implemented yet
    if(running)
    {

        return -1;


    }else
    {
        return -1;
    }
}

int sdrplay::biasTee(int on, int device_idx)
{

    Q_UNUSED(device_idx);

    if(running)
    {

            err = sdrplay_api_GetDeviceParams(chosenDevice->dev, &deviceParams);
            if (err != sdrplay_api_Success) {
                return -1;
            }

            // Enable Bias-T
            if (deviceParams && deviceParams->rxChannelA) {

                switch(chosenDevice->hwVer)
                {

                case SDRPLAY_RSP1A_ID :
                {
                    deviceParams->rxChannelA->rsp1aTunerParams.biasTEnable = on;
                    err = sdrplay_api_Update(chosenDevice->dev, chosenDevice->tuner,  sdrplay_api_Update_Rsp1a_BiasTControl, sdrplay_api_Update_Ext1_None);
                    break;
                }

                case SDRPLAY_RSP2_ID :
                {
                    deviceParams->rxChannelA->rsp2TunerParams.biasTEnable = on;
                    err = sdrplay_api_Update(chosenDevice->dev, chosenDevice->tuner, sdrplay_api_Update_Rsp2_BiasTControl, sdrplay_api_Update_Ext1_None);
                    break;
                }

                case SDRPLAY_RSPduo_ID :
                {
                    deviceParams->rxChannelA->rspDuoTunerParams.biasTEnable = on;
                    err = sdrplay_api_Update(chosenDevice->dev, chosenDevice->tuner,  sdrplay_api_Update_RspDuo_BiasTControl, sdrplay_api_Update_Ext1_None);
                    break;
                }

                case SDRPLAY_RSPdx_ID :
                {
                    deviceParams->devParams->rspDxParams.biasTEnable = on;
                    err =  sdrplay_api_Update(chosenDevice->dev, chosenDevice->tuner,  sdrplay_api_Update_None , sdrplay_api_Update_RspDx_BiasTControl );
                    break;
                }

                case SDRPLAY_RSP1B_ID :
                {
                    deviceParams->rxChannelA->rsp1aTunerParams.biasTEnable = on;
                    err = sdrplay_api_Update(chosenDevice->dev, chosenDevice->tuner,  sdrplay_api_Update_Rsp1a_BiasTControl, sdrplay_api_Update_Ext1_None);
                    break;
                }

                case SDRPLAY_RSPdxR2_ID :
                {
                    deviceParams->devParams->rspDxParams.biasTEnable = on;
                    err = sdrplay_api_Update(chosenDevice->dev, chosenDevice->tuner,   sdrplay_api_Update_None, sdrplay_api_Update_RspDx_BiasTControl);
                    break;

                }
            }

        } else {
                return -1;
        }


        if (err != sdrplay_api_Success) {
                return -1;
        }

    }

    biasT = on;

    return true;
}

QStringList sdrplay::deviceNames()
{
    QStringList result;

    float ver = 0.0;

    if ((err = sdrplay_api_Open()) != sdrplay_api_Success)
    {
        printf("sdrplay_api_Open failed %s\n", sdrplay_api_GetErrorString(err));
    }

    sdrplay_api_ApiVersion(&ver);

    sdrplay_api_LockDeviceApi();

    // Fetch list of available devices
    if ((err = sdrplay_api_GetDevices(devs, &ndev, sizeof(devs) /
                                                       sizeof(sdrplay_api_DeviceT))) != sdrplay_api_Success)
    {
        printf("sdrplay_api_GetDevices failed %s\n", sdrplay_api_GetErrorString(err));

    }

    else
    {

        QString device = "";

        for(unsigned int a = 0; a < ndev; a++)
        {

            switch(devs[a].hwVer)
            {

            case SDRPLAY_RSP1_ID :
                device = "RSP1";
                break;

            case SDRPLAY_RSP1A_ID :
                device = "RSP1A";
                break;

            case SDRPLAY_RSP2_ID :
                device = "RSP2";
                break;

            case SDRPLAY_RSPduo_ID :
                device = "RSPDuo";
                break;

            case SDRPLAY_RSPdx_ID :
                device = "RSPx";
                break;

            case SDRPLAY_RSP1B_ID :
                device = "RSP1B";
                break;

            case SDRPLAY_RSPdxR2_ID :
                device = "RSPdxR2";
                break;

            }


            result<< device + QString("-") + QString(devs[a].SerNo);

        }



    }

    sdrplay_api_UnlockDeviceApi();

    sdrplay_api_Close();

    return result;
}

void sdrplay::processCallback(short *xi, short *xq, unsigned int numSamples)
{
    for (unsigned int i = 0; i < numSamples; i++)
    {

        if(bufferPtr >= buffer.size())
        {
            sdr_callback(&buffer[0], bufferPtr);
            bufferPtr = 0;

        }
        buffer[bufferPtr++] = xi[i] * scale;
        buffer[bufferPtr++] = xq[i] * scale;
    }

}


float sdrplay::adjustGain(int gain)
{
    return (float)gain*100.0;
}

float sdrplay::getFFTGain()
{

    return 100.0f;
}
