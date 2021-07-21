#include "vfo.h"
#include "gnuradio/firfilter.h"
#include "iostream"

ZmqPublisher vfo::bind_publisher;
vfo::vfo(QObject *parent) : QObject(parent)
{

    gain = 0.01;

    avecpt = 0;
    val = 0.000001;
    one = 1.0;

    demodUSB = true;
    filterAudio = false;
    filterbw = 0;
    offsetbw = 0;
    mpVFOs = 0;

    laststageDecimate = false;
    emitFFT = false;
    scalecomp = 1;
    fir_decI = NULL;
    fir_decQ = NULL;
    osc_mix = NULL;
    osc_bfo = NULL;
    philbert = NULL;
    fir_usb = NULL;
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
}
void vfo::init(int samplesPerBuffer, bool bind)
{

    firfilter filt;
    osc_mix = new Oscillator(Fs, mixer_freq);

    int targetRate = Fs/(pow(2,decimateCount));
    int samplesOut = samplesPerBuffer/(pow(2,decimateCount));

    // samples per buffer should be multiple of 12000 otherwise we need to do one last decimation 4/5
    if(demodUSB && (samplesPerBuffer/12000)%2>0)
    {
        laststageDecimate = true;

        targetRate = (targetRate/5);
        samplesOut = (samplesOut/5);

        int firlen = 0;



        QVector<float> coeff = filt.low_pass(2,
                                            targetRate*5,
                                            targetRate/2,
                                            (double)targetRate/4,
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
    osc_bfo = new Oscillator(Fs/(pow(2,decimateCount)), offsetbw);


    if(filterbw >0)
    {


        QVector<float> coeff = filt.low_pass(2,
                                        targetRate,
                                        filterbw,
                                        (double)filterbw/4,
                                        firfilter::win_type::WIN_HAMMING,
                                        0);


        fir_usb=new FIR(coeff.length(), 0);
        for(int i=0;i<coeff.length();i++)
        {
         fir_usb->FIRSetPoint(i,coeff[i]);

        }
    }


    for(int a = 0; a<decimateCount; a++ )
    {

        int taps = 11;

         hdecimator[a] = new HalfBandDecimator(taps, Fs/(pow(2,a)));
    }


    delayT.setLength((125-1)/2);
    philbert = new FIRHilbert(125, samplesOut);



    transmit_usb.resize(samplesOut);

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

    if(!vfo::bind_publisher.connected && bind)
    {
        vfo::bind_publisher.setAddress(zmqAddress);
        vfo::bind_publisher.setBind(bind);
        vfo::bind_publisher.connect();
    }
    else if(!bind)
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
void vfo::setFs(int samplerate)
{
    Fs = samplerate;

}
void vfo::setDecimationCount(int count)
{
   decimateCount = count;
}

void vfo::setMixerFreq(double freq)
{

    mixer_freq = freq;

}

double vfo::getMixerFreq()
{

    return mixer_freq;

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

void vfo::process(const std::vector<cpx_typef> & samples)
{
    for(long unsigned int i=0;i<samples.size();++i)
    {

        //mix
        cpx_typef curr = osc_mix->_vector*samples.at(i);
        osc_mix->tick();

        decimate[0][i]=curr;
    }
    // decimate
    for(int i = 0; i<decimateCount; i++)
    {

          hdecimator[i]->decimate(decimate[i], decimate[i+1]);
    }

    if(mpVFOs != 0 && mpVFOs->length() > 0 )
    {


        for(int a = 0; a<mpVFOs->length(); a++)
        {
           vfo * pvfo = mpVFOs->at(a);

           pvfo->process(decimate[decimateCount]);

        }


    }
    else
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


    if(emitFFT)
    {
        emit fftData( decimate[decimateCount]);
    }


}



void vfo::usb_demod()
{

    for (long unsigned int i = 0; i < decimate[decimateCount].size(); i++) {

        cpx_typef curr = decimate[decimateCount][i];

        if(offsetbw > 1){

            curr = osc_bfo->_vector*curr;
            osc_bfo->tick();

        }

        float usb = 0;

        if(filterbw > 0)
        {
            usb = fir_usb->FIRUpdateAndProcess(delayT.update_dont_touch(curr.real()) - philbert->FIRUpdateAndProcess(curr.imag()));

        }

        else
        {
            usb = delayT.update_dont_touch(curr.real()) - philbert->FIRUpdateAndProcess(curr.imag());

        }

        transmit_usb[i] = usb * gain * 32768.0;

    }

}

void vfo::usb_decimdemod()
{


    int mark = 0;
    int check = 0;
    for (long unsigned int i = 0; i < decimate[decimateCount].size(); i++) {

        cpx_typef curr = decimate[decimateCount][i];

        // low pass

        if(offsetbw > 1){
              curr = osc_bfo->_vector*curr;
               osc_bfo->tick();
         }

        if(check== 0)
        {

            curr = cpx_typef(fir_decI->FIRUpdateAndProcess(curr.real()), fir_decQ->FIRUpdateAndProcess(curr.imag()));

            float usb = delayT.update_dont_touch(curr.real()) - philbert->FIRUpdateAndProcess(curr.imag());

            if(filterbw > 0)
            {
                usb = fir_usb->FIRUpdateAndProcess(usb);

            }

            transmit_usb[mark] = usb * gain * 32768.0;
            mark++;
            check++;

        }

        else if( check == 4)
        {
            fir_decI->FIRUpdate(curr.real());
            fir_decQ->FIRUpdate(curr.imag());

            check = 0;
        }
        else
        {
            fir_decI->FIRUpdate(curr.real());
            fir_decQ->FIRUpdate(curr.imag());
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

    if(demodUSB)
    {
        if(zmqBind)
        {
          vfo::bind_publisher.publish((unsigned char*)transmit_usb.data(), transmit_usb.size()*sizeof(short), zmqTopic);
        }
        else
        {
            connect_publisher.publish((unsigned char*)transmit_usb.data(), transmit_usb.size()*sizeof(short), zmqTopic);
        }
    }
    else
    {

        if(zmqBind)
        {
           bind_publisher.publish((unsigned char*)transmit_iq.data(), transmit_iq.size()*sizeof(char), zmqTopic);
        }
        else
        {
            connect_publisher.publish((unsigned char*)transmit_iq.data(), transmit_iq.size()*sizeof(char), zmqTopic);

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
