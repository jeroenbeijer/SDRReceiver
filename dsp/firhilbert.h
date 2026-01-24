#pragma once

#include <math.h>
#include <vector>
#include <QObject>

class FIRHilbert
{

public:
    FIRHilbert(int len);
    ~FIRHilbert();
    inline float FIRUpdateAndProcess(float sig)
    {
        // write new sample
        buff[ptr] = sig;
        buff[ptr + NumberOfPoints] = sig;

        // IMPORTANT: start at ptr+1, not ptr
        int start = ptr + 1;
        if (start >= NumberOfPoints)
            start = 0;

        float* b = &buff[start];

        float acc = 0.0f;
        for (int i = 1; i <= half; i += 2)
            acc += points[half + i] * (b[half + i] - b[half - i]);

        // advance pointer AFTER processing
        ptr++;
        if (ptr >= NumberOfPoints)
            ptr = 0;

        return acc;
    }

private:

    float *points;
    float *buff;
    int NumberOfPoints;
    int ptr;
    int half;
};
