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


#ifndef DSP_F_H
#define DSP_F_H

#endif // DSP_F_H

#include <math.h>
#include <vector>
#include <complex>
#include <QObject>

class FIR
{
public:
        FIR(int _NumberOfPoints, int queuesz);
        ~FIR();
        float  FIRUpdateAndProcess(float sig);
        float  FIRUpdateAndProcessHalfBand(float sig);
        float  FIRUpdateAndProcessHalfBandQueue(float sig);
        float  FIRUpdateAndProcessHalfBandQueue11(int tptr);
        float  FIRUpdateAndProcessHalfBandQueue23(int tptr);
        float  FIRUpdateAndProcessHalfBandQueue51(int tptr);

        void  FIRUpdate(float sig);
        void  FIRUpdateQueue(float sig);

        float  FIRUpdateAndProcess(float sig, float FractionOfSampleOffset);
        void  FIRSetPoint(int point, float value);

        float *points;
        float *buff;
        int NumberOfPoints;
        int buffsize;
        int ptr;
        float outsum;


        void FIRQueueBackToFront();
        float * queue;
        int queuePtr;
};

class FIRHilbert
{
public:
        FIRHilbert(int len, int Fs);
        ~FIRHilbert();
        double  FIRUpdateAndProcess(float sig);

        float *points;
        float *buff;
        int NumberOfPoints;
        int buffsize;
        int ptr;
        int M;
        float outsum;
};

class MovingAverage
{
public:
    MovingAverage(int number);
    ~MovingAverage();
    double Update(double sig);
    double UpdateSigned(double sig);
    void Zero();
    double Val;
private:
    int MASz;
    double MASum;
    double *MABuffer;
    int MAPtr;
};

template <class T>
class DelayThing
{
public:
    DelayThing()
    {
        setLength(12);
    }
    void setLength(int length)
    {
        length++;
        assert(length>0);
        buffer.resize(length);
        buffer_ptr=0;
        buffer_sz=buffer.size();
    }
    void update(T &data)
    {
        buffer[buffer_ptr]=data;
        buffer_ptr++;buffer_ptr%=buffer_sz;
        data=buffer[buffer_ptr];
    }
    T update_dont_touch(T data)
    {
        buffer[buffer_ptr]=data;
        buffer_ptr++;buffer_ptr%=buffer_sz;
        return buffer.at(buffer_ptr);
    }
    int findmaxpos(T &maxval)
    {
        int maxpos=0;
        maxval=buffer[buffer_ptr];
        for(int i=0;i<buffer_sz;i++)
        {
            if(buffer[buffer_ptr]>maxval)
            {
               maxval=buffer[buffer_ptr];
               maxpos=i;
            }
            buffer_ptr++;buffer_ptr%=buffer_sz;
        }
        return maxpos;
    }
private:
    QVector<T> buffer;
    int buffer_ptr;
    int buffer_sz;
};
