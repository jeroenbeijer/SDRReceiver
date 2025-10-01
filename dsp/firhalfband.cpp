#include "firhalfband.h"

FIRHalfband::FIRHalfband(int _NumberOfPoints, int queuesz) : FIR(_NumberOfPoints, queuesz) {


}

float  FIRHalfband::FIRUpdateAndProcessHalfBand(float sig)
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

float  FIRHalfband::FIRUpdateAndProcessHalfBandQueue(float sig)
{

    queue[queuePtr]=sig;
    queuePtr++;
    int tptr = queuePtr - NumberOfPoints;
    outsum=0;

    return (this->*processFunc_)(tptr);

}

void  FIRHalfband::FIRUpdateQueue(float sig)
{
    queue[queuePtr]=sig;
    queuePtr++;
}

inline float  FIRHalfband::FIRUpdateAndProcessHalfBandQueue11(int tptr) noexcept
{

    const float c0 = points[0];
    const float c2 = points[2];
    const float c4 = points[4];
    const float c5 = points[5];

    float acc =
        c0 * (queue[tptr]     + queue[tptr + 10]) +
        c2 * (queue[tptr + 2] + queue[tptr + 8])  +
        c4 * (queue[tptr + 4] + queue[tptr + 6])  +
        c5 *  queue[tptr + 5];

    outsum += acc;
    return outsum;

}

inline float FIRHalfband::FIRUpdateAndProcessHalfBandQueue23(int tptr) noexcept
{

    const float c0  = points[0];
    const float c2  = points[2];
    const float c4  = points[4];
    const float c6  = points[6];
    const float c8  = points[8];
    const float c10 = points[10];
    const float c11 = points[11];

    const float q0  = queue[tptr + 0];
    const float q1  = queue[tptr + 2];
    const float q2  = queue[tptr + 4];
    const float q3  = queue[tptr + 6];
    const float q4  = queue[tptr + 8];
    const float q5  = queue[tptr + 10];
    const float qC  = queue[tptr + 11];
    const float q6  = queue[tptr + 12];
    const float q7  = queue[tptr + 14];
    const float q8  = queue[tptr + 16];
    const float q9  = queue[tptr + 18];
    const float q10 = queue[tptr + 20];
    const float q11 = queue[tptr + 22];

    float acc = 0.0f;

    acc += c0  * (q0  + q11);
    acc += c2  * (q1  + q10);
    acc += c4  * (q2  + q9);
    acc += c6  * (q3  + q8);
    acc += c8  * (q4  + q7);
    acc += c10 * (q5  + q6);

    acc += c11 * qC;

    outsum += acc;
    return outsum;
}


inline float FIRHalfband::FIRUpdateAndProcessHalfBandQueue51(int tptr) noexcept
{

    const float c0  = points[0];
    const float c2  = points[2];
    const float c4  = points[4];
    const float c6  = points[6];
    const float c8  = points[8];
    const float c10 = points[10];
    const float c12 = points[12];
    const float c14 = points[14];
    const float c16 = points[16];
    const float c18 = points[18];
    const float c20 = points[20];
    const float c22 = points[22];
    const float c24 = points[24];
    const float c25 = points[25];

    const float q0  = queue[tptr + 0];
    const float q1  = queue[tptr + 2];
    const float q2  = queue[tptr + 4];
    const float q3  = queue[tptr + 6];
    const float q4  = queue[tptr + 8];
    const float q5  = queue[tptr + 10];
    const float q6  = queue[tptr + 12];
    const float q7  = queue[tptr + 14];
    const float q8  = queue[tptr + 16];
    const float q9  = queue[tptr + 18];
    const float q10 = queue[tptr + 20];
    const float q11 = queue[tptr + 22];
    const float q12 = queue[tptr + 24];
    const float qC  = queue[tptr + 25];
    const float q13 = queue[tptr + 26];
    const float q14 = queue[tptr + 28];
    const float q15 = queue[tptr + 30];
    const float q16 = queue[tptr + 32];
    const float q17 = queue[tptr + 34];
    const float q18 = queue[tptr + 36];
    const float q19 = queue[tptr + 38];
    const float q20 = queue[tptr + 40];
    const float q21 = queue[tptr + 42];
    const float q22 = queue[tptr + 44];
    const float q23 = queue[tptr + 46];
    const float q24 = queue[tptr + 48];
    const float q25 = queue[tptr + 50];

    float acc = 0.0f;

    acc += c0  * (q0  + q25);
    acc += c2  * (q1  + q24);
    acc += c4  * (q2  + q23);
    acc += c6  * (q3  + q22);
    acc += c8  * (q4  + q21);
    acc += c10 * (q5  + q20);
    acc += c12 * (q6  + q19);
    acc += c14 * (q7  + q18);
    acc += c16 * (q8  + q17);
    acc += c18 * (q9  + q16);
    acc += c20 * (q10 + q15);
    acc += c22 * (q11 + q14);
    acc += c24 * (q12 + q13);

    acc += c25 * qC;

    outsum += acc;
    return outsum;
}



void FIRHalfband::bindProcessFunc()
{
    if (NumberOfPoints == 11) {
        processFunc_ = &FIRHalfband::FIRUpdateAndProcessHalfBandQueue11;
    } else if (NumberOfPoints == 23) {
        processFunc_ = &FIRHalfband::FIRUpdateAndProcessHalfBandQueue23;
    } else if (NumberOfPoints == 51) {
        processFunc_ = &FIRHalfband::FIRUpdateAndProcessHalfBandQueue51;
    } else {
        processFunc_ = nullptr;
    }
}

void FIRHalfband::FIRQueueBackToFront()
{

    //queuePtr points to the next empy slot
    if(queuePtr >= NumberOfPoints)
    {
        std::copy(queue + ((queuePtr-1)-NumberOfPoints), queue + (queuePtr-1), queue);
    }

    queuePtr = NumberOfPoints;
}
