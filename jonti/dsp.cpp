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




void  FIR::FIRUpdate(float sig)
{
        buff[ptr]=sig;
        ptr++;ptr%=buffsize;
}



void  FIR::FIRSetPoint(int point, float value)
{

    if((point<0)||(point>=NumberOfPoints))return;
    points[point]=value;
}

MovingAverage::MovingAverage(int number)
{
    MASz=round(number);
    MASz=number;
    MASum=0;
    MABuffer=new double[MASz];
    for(int i=0;i<MASz;i++)MABuffer[i]=0;
    MAPtr=0;
    Val=0;
}

void MovingAverage::Zero()
{
    for(int i=0;i<MASz;i++)MABuffer[i]=0;
    MAPtr=0;
    Val=0;
    MASum=0;
}

double MovingAverage::Update(double sig)
{
    MASum=MASum-MABuffer[MAPtr];
    MASum=MASum+fabs(sig);
    MABuffer[MAPtr]=fabs(sig);
    MAPtr++;MAPtr%=MASz;
    Val=MASum/((double)MASz);
    return Val;
}

double MovingAverage::UpdateSigned(double sig)
{
    MASum=MASum-MABuffer[MAPtr];
    MASum=MASum+(sig);
    MABuffer[MAPtr]=(sig);
    MAPtr++;MAPtr%=MASz;
    Val=MASum/((double)MASz);
    return Val;
}

MovingAverage::~MovingAverage()
{
    if(MASz)delete [] MABuffer;
}


