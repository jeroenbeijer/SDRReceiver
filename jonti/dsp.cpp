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

#include "dsp.h"
#include <QDebug>

//---------------------------------------------------------------------------
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif
using namespace std;

FIR::FIR(int _NumberOfPoints, int queuesz)
{
        int i;
        points=0;buff=0;

        NumberOfPoints=_NumberOfPoints;
        buffsize=NumberOfPoints+1;
        points=new float[NumberOfPoints];
        for(i=0;i<NumberOfPoints;i++)points[i]=0;
        buff=new float[buffsize];
        for(i=0;i<buffsize;i++)buff[i]=0;
        ptr=0;
        outsum=0;

        queue = new float[queuesz + NumberOfPoints];

        for(i=0;i<queuesz + NumberOfPoints;i++)queue[i]=0;
        queuePtr = _NumberOfPoints;
}

FIR::~FIR()
{
        if(points)delete [] points;
        if(buff)delete [] buff;
        if(queue)delete [] queue;
}

float  FIR::FIRUpdateAndProcess(float sig)
{
        buff[ptr]=sig;
        ptr++;if(ptr>=buffsize)ptr=0;//ptr%=buffsize;
        int tptr=ptr;
        outsum=0;
        for(int i=0;i<NumberOfPoints;i++)
        {
                outsum+=points[i]*buff[tptr];
                tptr++;if(tptr>=buffsize)tptr=0;//tptr%=buffsize;
        }
        return outsum;
}


float  FIR::FIRUpdateAndProcessHalfBand(float sig)
{

    buff[ptr]=sig;
    ptr++;if(ptr>=buffsize)ptr=0;//ptr%=buffsize;
    int tptr=ptr;
    outsum=0;


    for(int i=0;i<NumberOfPoints;i++)
    {

            if(i%2 == 0 || i ==25)
            {
               outsum+=points[i]*buff[tptr];
            }
            tptr++;if(tptr>=buffsize)tptr=0;//tptr%=buffsize;
    }

    return outsum;
}

float  FIR::FIRUpdateAndProcessHalfBandQueue(float sig)
{

    queue[queuePtr]=sig;
    queuePtr++;
    int tptr = queuePtr - NumberOfPoints;

    outsum=0;

    switch(NumberOfPoints)
    {

      case 51 :

        outsum += points[0]*(queue[tptr] + queue[tptr + 50])
            + points[2]*(queue[tptr+2] + queue[tptr + 48])
            + points[4]*(queue[tptr+4] + queue[tptr + 46])
            + points[6]*(queue[tptr+6] + queue[tptr + 44])
            + points[8]*(queue[tptr+8] + queue[tptr + 42])
            + points[10]*(queue[tptr+10] + queue[tptr + 40])
            + points[12]*(queue[tptr+12] + queue[tptr + 38])
            + points[14]*(queue[tptr+14] + queue[tptr + 36])
            + points[16]*(queue[tptr+16] + queue[tptr + 34])
            + points[18]*(queue[tptr+18] + queue[tptr + 32])
            + points[20]*(queue[tptr+20] + queue[tptr + 30])
            + points[22]*(queue[tptr+22] + queue[tptr + 28])
            + points[24]*(queue[tptr+24] + queue[tptr + 26])
            + points[25]*(queue[tptr+25]) ;
        break;

     case 23:

        outsum += points[0]*(queue[tptr] + queue[tptr + 22])
            + points[2]*(queue[tptr+2] + queue[tptr + 20])
            + points[4]*(queue[tptr+4] + queue[tptr + 18])
            + points[6]*(queue[tptr+6] + queue[tptr + 16])
            + points[8]*(queue[tptr+8] + queue[tptr + 14])
            + points[10]*(queue[tptr+10] + queue[tptr + 12])
            + points[11]*(queue[tptr+11]) ;
        break;

    case 11:

       outsum += points[0]*(queue[tptr] + queue[tptr + 10])
           + points[2]*(queue[tptr+2] + queue[tptr + 8])
           + points[4]*(queue[tptr+4] + queue[tptr + 6])
           + points[5]*(queue[tptr+5]) ;
        break;
    }



    return outsum;
}
void  FIR::FIRUpdate(float sig)
{
        buff[ptr]=sig;
        ptr++;ptr%=buffsize;
}

void  FIR::FIRUpdateQueue(float sig)
{
        queue[queuePtr]=sig;
        queuePtr++;
}


void FIR::FIRQueueBackToFront()
{

        //queuePtr points to the next empy slot
        if(queuePtr >= NumberOfPoints)
        {
            std::copy(queue + ((queuePtr-1)-NumberOfPoints), queue + (queuePtr-1), queue);
        }

        queuePtr = NumberOfPoints;
}



void  FIR::FIRSetPoint(int point, float value)
{

    if((point<0)||(point>=NumberOfPoints))return;
    points[point]=value;
}

FIRHilbert::FIRHilbert(int len, int Fs)
{
        int i;
        points=0;buff=0;

        NumberOfPoints=len;

        points=new float[len];
        for(i=0;i<len;i++)points[i]=0;
        buff=new float[len];
        for(i=0;i<len;i++)buff[i]=0;
        ptr=0;
        outsum=0;

        QVector<float> tempCoeffs(len);

        float sumofsquares = 0;

        for (int n=0; n < len; n++) {
            if (n == len/2) {
                tempCoeffs[n] = 0;
            } else {
                tempCoeffs[n] = Fs / (M_PI * (n-len/2) ) * ( 1 - cos(M_PI * (n-len/2) ));


            }
            sumofsquares += tempCoeffs[n]*tempCoeffs[n];
        }
        double gain = sqrt(sumofsquares);

        for (int i=0; i < len; i++) {
            points[i] = tempCoeffs[len-i-1]/gain;
        }
}
double  FIRHilbert::FIRUpdateAndProcess(float sig)
{
        buff[ptr]=sig;
        ptr++;if(ptr>=NumberOfPoints)ptr=0;
        int tptr=ptr;
        outsum=0;

        for(int i=0;i<NumberOfPoints;i++)
        {
                outsum+=points[i]*buff[tptr];
                tptr++;if(tptr>=NumberOfPoints)tptr=0;
        }
        return outsum;
}

FIRHilbert::~FIRHilbert()
{
        if(points)delete [] points;
        if(buff)delete [] buff;
}
