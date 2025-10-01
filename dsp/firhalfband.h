#ifndef FIRHALFBAND_H
#define FIRHALFBAND_H

#include "jonti/dsp.h"

class FIRHalfband : public FIR
{
public:
    FIRHalfband(int _NumberOfPoints, int queuesz);

    void bindProcessFunc();
    float FIRUpdateAndProcess(float sig);
    float FIRUpdateAndProcessHalfBand(float sig);
    float FIRUpdateAndProcessHalfBandQueue(float sig);
    void FIRUpdateQueue(float sig);
    void FIRQueueBackToFront();

private:

    float (FIRHalfband::*processFunc_)(int) noexcept = 0;
    inline float  FIRUpdateAndProcessHalfBandQueue11(int tptr) noexcept;
    inline float  FIRUpdateAndProcessHalfBandQueue23(int tptr) noexcept;
    inline float  FIRUpdateAndProcessHalfBandQueue51(int tptr) noexcept;
};

#endif // FIRHALFBAND_H
