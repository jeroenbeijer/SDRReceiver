#include "vfo.h"
#include "gnuradio/firfilter.h"
#include "iostream"

vfo::vfo(QObject *parent) : QObject(parent)
{

    gain = 0.01;
    halfBandTaps = 11;

    avecpt = 0;
    val = 0.000001;
    one = 1.0;

    demodUSB = true;
    demodLSB = false;
    filterAudio = false;
    vfo_threads = true;
    filterbw = 0;
    offsetbw = 0;
    mpVFOs = 0;

    laststageDecimate = false;
    discard = 0;
    emitFFT = false;
    scalecomp = 1;
    fir_decI = NULL;
    fir_decQ = NULL;
    osc_mix = NULL;
    osc_bfo = NULL;
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
    if(osc_mix) delete osc_mix;
    if(osc_bfo) delete osc_bfo;
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
void vfo::init(int samplesPerBuffer, bool bind, bool threads, int lateDecimate)
{

    firfilter filt;
    osc_mix = new Oscillator(Fs, mixer_freq);

    int targetRate = Fs/(pow(2,decimateCount));
    int samplesOut = samplesPerBuffer/(pow(2,decimateCount));

    vfo_threads = threads;

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
    osc_bfo = new Oscillator(outputRate, offsetbw);


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

    delayT.setLength((125-1)/2);
    philbert = new FIRHilbert(125, samplesOut);

    transmit_usb.resize(samplesOut);
    if(demodLSB)
    {
      transmit_lsb.resize(samplesOut);
    }

    if(cstyle ==1)
    {

        transmit_iq.resize(samplesOut);
    }else
    {
        transmit_iq.resize(samplesOut*2);
    }

    decimate[0].resize(samplesPerBuffer);

    for(int a = 1; a<decimateCount+1; a++)
    {
        decimate[a].resize(decimate[a-1].size()/2);

    }

    if(!bind)
    {

        connect_publisher.setBind(false);
        connect_publisher.setAddress(zmqAddress);
        connect_publisher.connect();

    }

    zmqBind = bind;

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
void vfo::setOffsetBandwidth(double bw)
{
    offsetbw = bw;

}
void vfo::setFilterBandwidth(double bw)
{
    filterbw = bw;

}

void vfo::setGain(float g)
{

    gain = g;
}

void vfo::process(const QSharedPointer<std::vector<cpx_typef>> samples)
{

    cpx_typef curr;

    size_t sampleSize = samples->size();
    
    for (size_t i = 0; i < sampleSize; ++i) {
        decimate[0][i] = osc_mix->_vector * (*samples)[i];
        osc_mix->tick();
    }

    
    // decimate
    for(int i = 0; i<decimateCount; i++)
    {

        hdecimator[i]->decimate(decimate[i], decimate[i+1]);
    }


    sharedSamples = QSharedPointer<std::vector<cpx_typef>>::create(decimate[decimateCount]);

    if(emitFFT)
    {
        emit fftData(sharedSamples);
    }

    // thread push via signal and slot
    if(vfo_threads)
    {

        if(mpVFOs != 0 && mpVFOs->length() > 0 )
        {

             emit demodSubVFOData(sharedSamples, sharedSamples->size());
        }

    }else
    {

        if(mpVFOs != 0 && mpVFOs->length() > 0 )
        {
            for(int a = 0; a<mpVFOs->length(); a++)
            {
                vfo * pvfo = mpVFOs->at(a);

                pvfo->process(sharedSamples);

            }
        }
    }

    if(mpVFOs == 0 || mpVFOs->length() == 0 )
    {
        if(demodUSB)
        {
            if(!laststageDecimate)
            {
                usb_demod();
            }
            else
            {
                usb_decimdemod();
            }
        }
        else
        {
            compress();
        }

        transmitData();

    }



}



void vfo::usb_demod()
{

    float usb = 0.0;
    float lsb = 0.0;

    float delayImag = 0.0;
    float hilbertReal = 0.0;

    for (long unsigned int i = 0; i < decimate[decimateCount].size(); i++) {

        cpx_typef curr = decimate[decimateCount][i];

        if(offsetbw > 1){

            curr = osc_bfo->_vector*curr;
            osc_bfo->tick();

        }

        if(filterbw > 0)
        {

            if(demodLSB)
            {
                delayImag = delayT.update_dont_touch(curr.imag());
                hilbertReal = philbert->FIRUpdateAndProcess(curr.real());
                usb =  fir_usb->FIRUpdateAndProcess(delayImag + hilbertReal);
                lsb =  fir_lsb->FIRUpdateAndProcess(delayImag - hilbertReal);

            }else
            {
                usb = fir_usb->FIRUpdateAndProcess(delayT.update_dont_touch(curr.imag()) + philbert->FIRUpdateAndProcess(curr.real()));

            }
        }
        else
        {

            if(demodLSB)
            {
                delayImag = delayT.update_dont_touch(curr.imag());
                hilbertReal = philbert->FIRUpdateAndProcess(curr.real());
                usb =  delayImag + hilbertReal;
                lsb =  delayImag - hilbertReal;

            }else
            {
                usb = delayT.update_dont_touch(curr.imag()) + philbert->FIRUpdateAndProcess(curr.real());

            }

        }
        transmit_usb[i] = usb * gain * 32768.0;

        if(demodLSB)
        {
            transmit_lsb[i] = lsb * gain * 32768.0;
        }


    }

}


void vfo::usb_decimdemod()
{
    int mark = 0;
    int check = 0;

    float usb = 0.0;
    float lsb = 0.0;

    float delayImag = 0.0;
    float hilbertReal = 0.0;


    for (long unsigned int i = 0; i < decimate[decimateCount].size(); i++) {

        cpx_typef curr = decimate[decimateCount][i];

        // low pass

        if(offsetbw > 1){
            curr = osc_bfo->_vector*curr;
            osc_bfo->tick();
        }

        if(check== 0)
        {

            curr = cpx_typef(fir_decQ->FIRUpdateAndProcess(curr.real()),fir_decI->FIRUpdateAndProcess(curr.imag()));

            if(filterbw > 0)
            {

                if(demodLSB)
                {
                    delayImag = delayT.update_dont_touch(curr.imag());
                    hilbertReal = philbert->FIRUpdateAndProcess(curr.real());
                    usb =  fir_usb->FIRUpdateAndProcess(delayImag + hilbertReal);
                    lsb =  fir_lsb->FIRUpdateAndProcess(delayImag - hilbertReal);

                }else
                {

                    usb = fir_usb->FIRUpdateAndProcess(delayT.update_dont_touch(curr.imag()) + philbert->FIRUpdateAndProcess(curr.real()));

                }
            }
            else
            {
               if(demodLSB)
                {
                    delayImag = delayT.update_dont_touch(curr.imag());
                    hilbertReal = philbert->FIRUpdateAndProcess(curr.real());
                    usb =  delayImag + hilbertReal;
                    lsb =  delayImag - hilbertReal;
                }else
                {
                    usb = delayT.update_dont_touch(curr.imag()) + philbert->FIRUpdateAndProcess(curr.real());
                }

            }
            transmit_usb[mark] = usb * gain * 32768.0;

            if(demodLSB)
            {
                transmit_lsb[mark] = lsb * gain * 32768.0;
            }

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

}

void vfo::compress()
{


    if(cstyle==1)
    {

        // drop 4 LSB och each arm and combine into 1 byte
        for (long unsigned int i = 0; i < decimate[decimateCount].size(); i++) {

            cpx_typef curr = decimate[decimateCount][i];

            signed char real = (curr.real()/scalecomp)*128;
            signed char imag = (curr.imag()/scalecomp)*128;

            transmit_iq[i] = ( (real & 0xF0) | (imag & 0xF0) >> 4 );

        }

    }

    else
    {

        for (long unsigned int i = 0; i < decimate[decimateCount].size(); i++) {

            cpx_typef curr = decimate[decimateCount][i];

            transmit_iq[2*i] =  curr.real()*128;
            transmit_iq[2*i+1]= curr.imag()*128;

        }

    }

}

void vfo::transmitData(){


    if(demodUSB || demodLSB)
    {

            if(zmqBind)
            {
                if(demodUSB)
                {

                    sharedTransmit = QSharedPointer<std::vector<short>>::create(transmit_usb);

                    emit transmitData(sharedTransmit, transmit_usb.size()*sizeof(short), zmqTopic, outputRate);
                }
                if(demodLSB)
                {

                    sharedTransmit = QSharedPointer<std::vector<short>>::create(transmit_lsb);

                    emit transmitData(sharedTransmit, transmit_lsb.size()*sizeof(short), zmqTopicLSB, outputRate);
                }

            }
            else
            {
                if(demodUSB)
                {
                    connect_publisher.publish((unsigned char*)transmit_usb.data(), transmit_usb.size()*sizeof(short), zmqTopic, outputRate);
                }
                if(demodLSB)
                {
                    connect_publisher.publish((unsigned char*)transmit_lsb.data(), transmit_lsb.size()*sizeof(short), zmqTopicLSB, outputRate);
                }

            }
    }


    else if(zmqTopic.length()>0)
    {

        if(zmqBind)
        {
            //bind_publisher.publish((unsigned char*)transmit_iq.data(), transmit_iq.size()*sizeof(char), zmqTopic, outputRate);
        }
        else
        {
            connect_publisher.publish((unsigned char*)transmit_iq.data(), transmit_iq.size()*sizeof(char), zmqTopic,outputRate);

            std::cout << "xmit data " << "\r\n" << std::flush;

        }
    }

}

void vfo::setCompressonStyle(int st)
{

    cstyle = st;

}

void vfo::setScaleComp(int scale)
{

    scalecomp = scale;

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
void vfo::setFilter(bool filter, int bw )
{

    filterAudio = filter;
    filterbw = bw;
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
void vfo::demodVFOData(const QSharedPointer<std::vector<cpx_typef>> samples, int len)
{

    Q_UNUSED(len);
    process(samples);
}
