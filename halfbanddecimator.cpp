#include "halfbanddecimator.h"

HalfBandDecimator::HalfBandDecimator(int taps, int inlen)
{

    fir_i=new FIR(taps, inlen);
    fir_q=new FIR(taps, inlen);


    switch(taps){

    case 51:
        for(int i=0;i<taps;i++)
        {
            fir_i->FIRSetPoint(i,hbcoeff51[i]);
            fir_q->FIRSetPoint(i,hbcoeff51[i]);
        }
        break;
    case 11:
        for(int i=0;i<taps;i++)
        {
            fir_i->FIRSetPoint(i,hbcoeff11[i]);
            fir_q->FIRSetPoint(i,hbcoeff11[i]);
        }
        break;
    case 23:
        for(int i=0;i<taps;i++)
        {
            fir_i->FIRSetPoint(i,hbcoeff23[i]);
            fir_q->FIRSetPoint(i,hbcoeff23[i]);
        }
        break;

    }

}
HalfBandDecimator::~HalfBandDecimator()
{
    delete fir_i;
    delete fir_q;
}

void HalfBandDecimator::decimate(const std::vector<cpx_typef> &in, std::vector<cpx_typef> &out)
{
    int step = 0;
    // decimate half band 1
    int size = (int)in.size();
    for(int i=0;i<size;++i)
    {

        cpx_typef curr;

        if(i%2==0)
        {
            curr = cpx_typef(fir_i->FIRUpdateAndProcessHalfBandQueue(in[i].real()), fir_q->FIRUpdateAndProcessHalfBandQueue(in[i].imag()));
            out[step]=curr;

            step++;

        }else
        {

            fir_i->FIRUpdateQueue(in[i].real());
            fir_q->FIRUpdateQueue(in[i].imag());

        }
    }

    fir_i->FIRQueueBackToFront();
    fir_q->FIRQueueBackToFront();

}
