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
SOFTWARE.

Leaving this permission notice in, portions of this code orginally came from Jonathan.

*/
#include <cstring>
#include "radio.h"

Radio::Radio(QObject *parent)  : QObject(parent), bufferPool(10)
{
    do_demod_dispatcher_cancel=false;
    buffers_head_ptr=0;
    buffers_tail_ptr=0;
    buffers_used=0;
    count=0;

    active = false;

    floats.resize(256);
    const float scale = 1.0;// /127.0;

    for (int i = 0; i < 256; i++)
    {
        floats[i] = (i - 127)*scale;
    }

}

Radio::~Radio()
{
    for(int a = 0; a<mpVFOs->length(); a++)
    {
        vfo * pvfo = mpVFOs->at(a);
        delete pvfo;
    }


}
void Radio::setVFOs(QVector<vfo*> * vfos)
{

    mpVFOs = vfos;

}
void Radio::setDCCorrection(bool correct)
{

    correctDC = correct;
}

QStringList Radio::deviceNames()
{

    QStringList base;

    return base;
}

void Radio::demodData(const float* data,int len)
{
    
    long long unsigned int sampleCount = len / 2;

    if (samples.size() != sampleCount) 
    {
        samples.resize(sampleCount); 
    }

    for(int i=0;i<len/2;++i)
    {

        //convert to complex float
        cpx_typef curr = cpx_typef(data[2*i],data[2*i+1]);

        if(correctDC)
        {
            avept=avept*(1.0f-0.000001f)+0.000001f*curr;
            curr-=avept;
        }

        samples[i] = curr;
    }


    ComplexSampleBuffer* buf =
        bufferPool.acquireBuffer(mpVFOs->length());

    buf->samples.resize(samples.size());
    std::memcpy(buf->samples.data(),
                samples.data(),
                samples.size() * sizeof(cpx_typef));

    for(int a = 0; a<mpVFOs->length(); a++)
    {

        vfo * pvfo = mpVFOs->at(a);

        pvfo->process(buf);
    }


    int fftnt = 16;

    if(++count == fftnt && emitFFT)
    {

        if (!sharedSamples || sharedSamples->size() != sampleCount) {
            sharedSamples = QSharedPointer<std::vector<cpx_typef>>::create(samples);
        }
        else {
            *sharedSamples = samples;
        }
        emit fftData(sharedSamples);
        count=0;
    }


}

void Radio::fftVFOSlot(QString topic)
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


QFuture<bool> Radio::startDispatcher()
{
    return QtConcurrent::run([this]() -> bool {
        return this->demod_dispatcher();
    });
}

bool Radio::demod_dispatcher()
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

        //cycle buffers
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

