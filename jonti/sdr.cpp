/* The MIT License (MIT)

Copyright (c) 2015-2019 Jonathan Olds

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#include "sdr.h"
#include <thread>
#include <qstringlist.h>
#include <qstring.h>
#include <QtConcurrent/QtConcurrent>
#include <iostream>
#include "qvector.h"

sdr::sdr(QObject *parent) : QObject(parent)
{

    rtldev = NULL;

    do_demod_dispatcher_cancel=false;
    buffers_head_ptr=0;
    buffers_tail_ptr=0;
    buffers_used=0;

    active = false;

    floats.resize(256);
    const float scale = 1.0;// /127.0;

    for (int i = 0; i < 256; i++)
    {
          floats[i] = (i - 127)*scale;
    }


}

sdr::~sdr()
{


}

bool sdr::OpenRtl(int device_idx)
{

    int open_res = rtlsdr_open(&rtldev,device_idx);
    if (open_res != 0)
    {
        return false;
    }

    return true;

}

void sdr::StartRtl(int samplerate, int frequency, int buflen, int gain)
{

    do_demod_dispatcher_cancel=false;
    active=false;
    buffers_head_ptr=0;
    buffers_tail_ptr=0;
    buffers_used=0;


    //reset buffer and go
    rtlsdr_reset_buffer(rtldev);
    future_demod_dispatcher = QtConcurrent::run(this,&sdr::demod_dispatcher);

    rtlsdr_set_sample_rate(rtldev, samplerate);
    rtlsdr_set_center_freq(rtldev, frequency);
    rtlsdr_set_tuner_gain_mode(rtldev,1);
    rtlsdr_set_tuner_gain(rtldev, gain);
    rtlsdr_set_agc_mode(rtldev,0);

    future_rtlsdr_callback = QtConcurrent::run(rtlsdr_read_async, rtldev,(rtlsdr_read_async_cb_t)rtlsdr_callback_dispatcher, this,0,buflen);//16384*2);

    active = true;

}


void sdr::rtlsdr_callback(unsigned char *buf, uint32_t len)
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

    //advertive we have filled a buffer
    buffers_mut.lock();
    ++buffers_used;
    buffers_not_empty.wakeAll();
    buffers_mut.unlock();

    //goto next buffer
    buffers_head_ptr++;

    return;

}

bool sdr::demod_dispatcher()
{

    while(true)
    {

        buffers_mut.lock();
        if(buffers_used==0)
        {
            buffers_not_empty.wait(&buffers_mut);
        }
        buffers_mut.unlock();

        //check if reason for waking is to cancel
        if(do_demod_dispatcher_cancel)break;

        //cycle beffers
        buffers_tail_ptr%=N_BUFFERS;

        //load buffer ptr and size
        float *buffptr=buffers[buffers_tail_ptr].data();
        int buffer_size=buffers_size_valid[buffers_tail_ptr];

        //pass the data to someone else. use a directconnection here
        emit audio_signal_out(buffptr,buffer_size);

        //goto next buffer
        ++buffers_tail_ptr;

        buffers_mut.lock();
        --buffers_used;
        buffers_mut.unlock();

    }

    return do_demod_dispatcher_cancel;

}


bool sdr::StopAndCloseRtl()
{
    bool result=false;
    if(active)
    {

           result = StopRtl();
           if(rtlsdr_close(rtldev)==0)rtldev=NULL;
    }
    do_demod_dispatcher_cancel=false;
    active=false;
    buffers_head_ptr=0;
    buffers_tail_ptr=0;
    buffers_used=0;
    return result;
}

bool sdr::StopRtl()
{

    //stop rtlsdr_callback
    int result=1;
    if(!future_rtlsdr_callback.isFinished())
    {
        qDebug()<<"stopping rtlsdr_callback";
        rtlsdr_cancel_async(rtldev);
        future_rtlsdr_callback.waitForFinished();
        result=future_rtlsdr_callback.result();
        if(result)qDebug()<<"Error stopping thread";
        std::this_thread::sleep_for(std::chrono::milliseconds(250));//100ms seems to be the min
        qDebug()<<"rtlsdr_callback stopped";
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
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        qDebug()<<"demod_dispatcher stopped";
    }

    //clear return buffer state
    //qDebug()<<"clearing buffers";
    buffers_head_ptr=0;
    buffers_tail_ptr=0;
    buffers_used=0;
    for(int i=0;i<N_BUFFERS;i++)buffers[i].clear();

    if(result)return false;
    return true;
}




QStringList sdr::deviceNames()
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

