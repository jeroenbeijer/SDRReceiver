#include "firhilbert.h"

FIRHilbert::FIRHilbert(int len)
{
    NumberOfPoints = len;
    half = len / 2;
    ptr = 0;
    points = new float[len];
    buff   = new float[2 * len];

    for (int i = 0; i < len; ++i)
        points[i] = 0.0f;

    for (int i = 0; i < 2 * len; ++i)
        buff[i] = 0.0f;

    const int half = len / 2;

    QVector<float> tempCoeffs(len);
    double sumofsquares = 0.0;

    // Ideal Hilbert impulse response
    for (int n = 0; n < len; ++n)
    {
        int k = n - half;

        if (k == 0 || (k & 1) == 0)
            tempCoeffs[n] = 0.0f;
        else
            tempCoeffs[n] = 2.0f / (float)(M_PI * k);

        sumofsquares += tempCoeffs[n] * tempCoeffs[n];
    }

    // Normalize energy (CRITICAL)
    const float gain = std::sqrt(sumofsquares);

    // Reverse to match circular buffer access
    for (int i = 0; i < len; ++i)
        points[i] = tempCoeffs[len - 1 - i] / gain;
}




FIRHilbert::~FIRHilbert()
{
    if(points)delete [] points;
    if(buff)delete [] buff;
}
