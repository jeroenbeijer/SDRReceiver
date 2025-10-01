#include "sidebandselector.h"
#include <cstdlib>
#include "jonti/dsp.h"

SidebandSelector::SidebandSelector(int blockSize, Sideband side)
    : m_block(blockSize),
    m_side(side),
    m_fftSize(2 * blockSize),
    m_overlap(blockSize, {0.0f, 0.0f}),
    m_fftBuf(m_fftSize),
    m_fwd(nullptr),
    m_inv(nullptr)
{
    m_fwd = kiss_fft_alloc(m_fftSize, 0, nullptr, nullptr);
    m_inv = kiss_fft_alloc(m_fftSize, 1, nullptr, nullptr);
}

SidebandSelector::~SidebandSelector()
{
    if (m_fwd) free(m_fwd);
    if (m_inv) free(m_inv);
}

void SidebandSelector::process(
    const std::vector<std::complex<float>>& inIQ,
    std::vector<short int>& outReal,
    float gain,
    FIR * pFIR
    )
{
    // safety check
    if ((int)inIQ.size() < m_block)
        return;

    // overlap
    for (int i = 0; i < m_block; ++i)
    {
        m_fftBuf[i].r = m_overlap[i].real();
        m_fftBuf[i].i = m_overlap[i].imag();
    }

    for (int i = 0; i < m_block; ++i)
    {
        m_fftBuf[i + m_block].r = inIQ[i].real();
        m_fftBuf[i + m_block].i = inIQ[i].imag();
    }

    // save overlap
    for (int i = 0; i < m_block; ++i)
        m_overlap[i] = inIQ[i];

    // --- forward FFT ---
    kiss_fft(m_fwd, m_fftBuf.data(), m_fftBuf.data());


    const int half = m_fftSize / 2;
    const int N = m_fftSize;

    if (m_side == Sideband::Positive)
    {
        // zero negative freqs
        for (int k = half + 1; k < N; ++k) {
            m_fftBuf[k].r = 0.0f;
            m_fftBuf[k].i = 0.0f;
        }

        // Nyquist
        m_fftBuf[half].r = 0.0f;
        m_fftBuf[half].i = 0.0f;
    }
    else // Negative
    {
        // zero positive freqs (including DC)
        for (int k = 0; k < half; ++k) {
            m_fftBuf[k].r = 0.0f;
            m_fftBuf[k].i = 0.0f;
        }

        // Nyquist
        m_fftBuf[half].r = 0.0f;
        m_fftBuf[half].i = 0.0f;
    }

    // --- inverse FFT ---
    kiss_fft(m_inv, m_fftBuf.data(), m_fftBuf.data());

    // --- output ---
    outReal.resize(m_block);
    const float scale = 1.0f / m_fftSize;

    for (int i = 0; i < m_block; ++i)
    {

        if(pFIR)
        {
            outReal[i] = pFIR->FIRUpdateAndProcess(m_fftBuf[i + m_block].r * scale) * 32768.0 * gain;
        }else
        {
            outReal[i] = (m_fftBuf[i + m_block].r * scale) * 32768.0 * gain ;
        }


    }
}
