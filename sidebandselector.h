#pragma once

#include <vector>
#include <complex>
#include "kiss_fft130/kiss_fft.h"

class FIR;

class SidebandSelector
{
public:

    enum class Sideband
    {
        Positive,
        Negative
    };

    explicit SidebandSelector(int blockSize, Sideband side);
    ~SidebandSelector();

    void process(
        const std::vector<std::complex<float>>& inIQ,
        std::vector<short int>& outReal,
        float gain,
        FIR * pFIR = NULL
        );

private:
    int m_block;
    Sideband m_side;
    int m_fftSize;

    std::vector<std::complex<float>> m_overlap;
    std::vector<kiss_fft_cpx> m_fftBuf;

    kiss_fft_cfg m_fwd;
    kiss_fft_cfg m_inv;


};

