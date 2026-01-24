#include <cstring>
#include "vfo.h"
#include "gnuradio/firfilter.h"
#include "iostream"
#include "TxMessage.h"
#include "audiobufferpool.h"
#include "audiobufferpoolaccess.h"

vfo::vfo(QObject *parent) : QObject(parent), bufferPool(10)
{

    gain = 0.01;
    halfBandTaps = 11;

    avecpt = 0;

    demodUSB = true;
    demodLSB = false;
    filterbw = 0;
    mpVFOs = 0;

    laststageDecimate = false;
    samplesOut=0;
    discard = 0;
    emitFFT = false;
    fir_decI = NULL;
    fir_decQ = NULL;
    pOsc_mix = NULL;
    philbert = NULL;
    fir_usb = NULL;
    fir_lsb = NULL;

}

vfo::~vfo()
{

    for(int a = 0; a<decimateCount; a++ )
    {

        delete hdecimator[a];
    }

    if(fir_decI) delete fir_decI;
    if(fir_decQ) delete fir_decQ;
    if(pOsc_mix) delete pOsc_mix;
    if(philbert) delete philbert;
    if(fir_usb) delete fir_usb;
    if(fir_lsb) delete fir_lsb;

    if(mpVFOs != 0 && mpVFOs->length() > 0 )
    {
        for(int a = 0; a <mpVFOs->length(); a++)
        {
            delete mpVFOs->at(a);
        }
    }

}

void vfo::init(int samplesPerBuffer, int lateDecimate)
{

    firfilter filt;

    int targetRate = Fs/(pow(2,decimateCount));
    samplesOut = samplesPerBuffer/(pow(2,decimateCount));

    if(zmqTopicLSB.length()>0)
    {
        demodLSB = true;
    }

    // check for late decimate 4/5 or 5/6
    if(demodUSB && lateDecimate >0)
    {
        laststageDecimate = true;
        discard = lateDecimate-1;

        targetRate = (targetRate/lateDecimate);
        samplesOut = (samplesOut/lateDecimate);

        int firlen = 0;

        QVector<float> coeff = filt.low_pass(2,
                                             targetRate*lateDecimate,
                                             targetRate/2,
                                             (double)targetRate/(lateDecimate-1),
                                             firfilter::win_type::WIN_HAMMING,
                                             0);
        firlen = coeff.length();
        fir_decI = new FIR(firlen, 0);
        fir_decQ = new FIR(firlen, 0);

        for(int i=0;i<firlen;i++)
        {
            fir_decI->FIRSetPoint(i,coeff[i]);
            fir_decQ->FIRSetPoint(i,coeff[i]);
        }

    }
    outputRate = targetRate;
    pOsc_mix = new Oscillator(Fs, mixer_freq);

    if(filterbw >0)
    {

        QVector<float> coeff = filt.low_pass(2,
                                             targetRate,
                                             filterbw,
                                             (double)filterbw/4,
                                             firfilter::win_type::WIN_HAMMING,
                                             0);


        fir_usb=new FIR(coeff.length(), 0);
        if(demodLSB)
        {
            fir_lsb=new FIR(coeff.length(), 0);
        }
        for(int i=0;i<coeff.length();i++)
        {
            fir_usb->FIRSetPoint(i,coeff[i]);
            if(demodLSB)
            {
                fir_lsb->FIRSetPoint(i,coeff[i]);
            }

        }
    }


    for(int a = 0; a<decimateCount; a++ )
    {
        hdecimator[a] = new HalfBandDecimator(halfBandTaps, Fs/(pow(2,a)));
    }

    if(demodUSB)
    {

      delayT.setLength((81-1)/2);
      philbert = new FIRHilbert(81);

    }

    decimate[0].resize(samplesPerBuffer);

    for(int a = 1; a<decimateCount+1; a++)
    {
        decimate[a].resize(decimate[a-1].size()/2);

    }
}

void vfo::setZmqAddress(QString address)
{

    zmqAddress = address;

}

void vfo::setZmqTopic(QString top)
{

    zmqTopic = top;

}

void vfo::setZmqTopicLSB(QString top)
{

    zmqTopicLSB = top;

}

void vfo::setFs(int samplerate)
{
    Fs = samplerate;

}

void vfo::setDecimationCount(int count)
{
    decimateCount = count;
}

void vfo::setHalfbandTaps(int taps)
{
    halfBandTaps = taps;
}

void vfo::setMixerFreq(double freq)
{

    mixer_freq = freq;

}
void vfo::setCenterFreq(double freq)
{

    center_freq = freq;

}

double vfo::getMixerFreq()
{

    return mixer_freq;

}
double vfo::getCenterFreq()
{

    return center_freq;

}

int vfo::getOutRate()
{

    return Fs/(pow(2, decimateCount));

}

void vfo::setFilterBandwidth(double bw)
{
    filterbw = bw;

}

void vfo::setGain(float g)
{

    gain = g;
}


void vfo::process(ComplexSampleBuffer * pBuff)
{

    cpx_typef curr;

    size_t sampleSize = pBuff->samples.size();

    for (size_t i = 0; i < sampleSize; ++i) {
        decimate[0][i] = pOsc_mix->_vector * pBuff->samples[i];
        pOsc_mix->tick();
    }

    // release SampleBuffer
    pBuff->remaining.fetch_sub(1, std::memory_order_acq_rel);


    // decimate
    for(int i = 0; i<decimateCount; i++)
    {
        hdecimator[i]->decimate(decimate[i], decimate[i+1]);
    }

    if(emitFFT)
    {
        sharedSamples = QSharedPointer<std::vector<cpx_typef>>::create(decimate[decimateCount]);
        emit fftData(sharedSamples);
    }

    // pass data to threads
    if(mpVFOs != NULL && mpVFOs->length() > 0  )
    {

        ComplexSampleBuffer* buf =
            bufferPool.acquireBuffer(mpVFOs->length());

        buf->samples.resize(decimate[decimateCount].size());
        std::memcpy(buf->samples.data(),
                    decimate[decimateCount].data(),
                    decimate[decimateCount].size() * sizeof(cpx_typef));


        if(mpVFOs != 0 && mpVFOs->length() > 0 )
        {

            for(int a = 0; a<mpVFOs->length(); a++)
            {
                vfo * pvfo = mpVFOs->at(a);

                pvfo->process(buf);

            }
        }

    }

    if(mpVFOs == 0 || mpVFOs->length() == 0 )
    {
        if(demodUSB)
        {
            if(!laststageDecimate)
            {
                ssbDemod();
            }
            else
            {
                finalDecim();
            }
        }
    }
}


void vfo::ssbDemod()
{

    QSharedPointer<AudioSampleBuffer> bufUsb = NULL;
    QSharedPointer<AudioSampleBuffer> bufLsb = NULL;
    bufUsb = audioPool().acquire();

    bufUsb->data.resize(samplesOut);
    bufUsb->len = bufUsb->data.size()*sizeof(short);
    bufUsb->topic = zmqTopic;
    bufUsb->sampleRate = outputRate;

    if(demodLSB)
    {
        bufLsb = audioPool().acquire();

        bufLsb->data.resize(samplesOut);
        bufLsb->len = bufLsb->data.size()*sizeof(short);
        bufLsb->topic = zmqTopicLSB;
        bufLsb->sampleRate = outputRate;

    }

    float usb = 0.0;
    float lsb = 0.0;

    float delayImag = 0.0;
    float hilbertReal = 0.0;

    for (long unsigned int i = 0; i < samplesOut; i++) {

        cpx_typef curr = decimate[decimateCount][i];

        delayImag = delayT.update_dont_touch(curr.imag());
        hilbertReal = philbert->FIRUpdateAndProcess(curr.real());
        if(filterbw > 0)
        {
            usb =  fir_usb->FIRUpdateAndProcess(delayImag + hilbertReal);

        }else
        {
            usb =  delayImag + hilbertReal;
        }

        bufUsb->data[i] = gain * 32768.0 * usb;

        if(demodLSB)
        {
            if(filterbw > 0)
            {
                lsb =  fir_lsb->FIRUpdateAndProcess(delayImag - hilbertReal);
            }
            else
            {
                lsb = delayImag - hilbertReal;
            }

            bufLsb->data[i] = gain * 32768.0 * lsb;

        }
    }

    pAudioBufferQueue->push(bufUsb);
    emit bufferReady();

    if(demodLSB)
    {
        pAudioBufferQueue->push(bufLsb);
        emit bufferReady();
    }

}
void vfo::finalDecim()
{
    int mark = 0;
    int check = 0;

    // filter and decimate
    for (long unsigned int i = 0; i < decimate[decimateCount].size(); i++)
    {

        cpx_typef curr = decimate[decimateCount][i];

        if(check== 0)
        {

            curr = cpx_typef(fir_decQ->FIRUpdateAndProcess(curr.real()),fir_decI->FIRUpdateAndProcess(curr.imag()));

            decimate[decimateCount][mark] = curr;

            mark++;
            check++;

        }

        else if( check == discard)
        {
            fir_decI->FIRUpdate(curr.imag());
            fir_decQ->FIRUpdate(curr.real());

            check = 0;
        }
        else
        {
            fir_decI->FIRUpdate(curr.imag());
            fir_decQ->FIRUpdate(curr.real());
            check++;
        }
    }

    ssbDemod();
}

void vfo::setDemodUSB(bool usb)
{

    demodUSB = usb;
}

bool vfo::getDemodUSB()
{
    return demodUSB;
}

QString vfo::getZmqTopic()
{
    return zmqTopic;
}

void vfo::setVFOs(QVector<vfo*> * vfos)
{
    mpVFOs = vfos;
}

void vfo::fftVFOSlot(QString topic)
{

    if(topic.compare(zmqTopic) ==0)
    {

        emitFFT = true;

        FFTcount = 0;
    }
    else
    {
        emitFFT = false;

        FFTcount = 0;
    }

}

void vfo::setQueue(AudioSampleBufferQueue* queue)
{
    pAudioBufferQueue = queue;
}
