/*this code comes from gnuradio which is covered by the GNU GENERAL PUBLIC LICENSE

https://www.gnu.org/licenses/gpl-3.0.en.html */

#include "firfilter.h"
#include <iostream>
#include "math.h"


#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif
double firfilter::filter(double in)
{


    double sum = 0;
    int i;
    for (i = 0; i < M; i++)
    {
          xv[i] = xv[i+1];

    }
    xv[M] = in;
    sum = 0.0;
    for (i = 0; i <= M; i++)
    {
               sum += (coeffs[i] * xv[i]);

    }
    return sum;



}
firfilter::firfilter()
{

    xv.resize(241);
    M = 240;

}

void firfilter::setTaps(QVector<double> taps)
{

    coeffs.resize(taps.length());

    for(int a = 0; a<taps.length(); a++){
        coeffs[a] = taps[a];
    }

    M = taps.length()-1;
    xv.resize(taps.length());


}
firfilter::~firfilter()
{

}


QVector<float> firfilter::low_pass(double gain,
                               double sampling_freq,
                               double cutoff_freq,      // Hz center of transition band
                               double transition_width, // Hz width of transition band
                               win_type window_type,
                               double beta) // used only with Kaiser
{
    sanity_check_1f(sampling_freq, cutoff_freq, transition_width);

    int ntaps = compute_ntaps(sampling_freq, transition_width, window_type, beta);

    // construct the truncated ideal impulse response
    // [sin(x)/x for the low pass case]

    QVector<float> taps(ntaps);
    QVector<float> w = QVector<float>::fromStdVector(window(window_type, ntaps));

    int M = (ntaps - 1) / 2;
    double fwT0 = 2 * M_PI * cutoff_freq / sampling_freq;

    for (int n = -M; n <= M; n++) {
        if (n == 0)
            taps[n + M] = fwT0 / M_PI * w[n + M];
        else {
            // a little algebra gets this into the more familiar sin(x)/x form
            taps[n + M] = sin(n * fwT0) / (n * M_PI) * w[n + M];
        }
    }

    // find the factor to normalize the gain, fmax.
    // For low-pass, gain @ zero freq = 1.0

    double fmax = taps[0 + M];
    for (int n = 1; n <= M; n++)
        fmax += 2 * taps[n + M];

    gain /= fmax; // normalize

    for (int i = 0; i < ntaps; i++)
        taps[i] *= gain;

    return taps;
}

int firfilter::compute_ntaps(double sampling_freq,
                          double transition_width,
                          win_type window_type,
                          double beta)
{
    double a = max_attenuation(window_type, beta);
    int ntaps = (int)(a * sampling_freq / (22.0 * transition_width));
    if ((ntaps & 1) == 0) // if even...
        ntaps++;          // ...make odd

    return ntaps;
}


void firfilter::sanity_check_1f(double sampling_freq,
                             double fa, // cutoff freq
                             double transition_width)
{
    if (sampling_freq <= 0.0)
        throw std::out_of_range("firdes check failed: sampling_freq > 0");

    if (fa <= 0.0 || fa > sampling_freq / 2)
        throw std::out_of_range("firdes check failed: 0 < fa <= sampling_freq / 2");

    if (transition_width <= 0)
        throw std::out_of_range("firdes check failed: transition_width > 0");
}

std::vector<float> firfilter::window(win_type type, int ntaps)
{
    return build(type, ntaps);
}

double firfilter::max_attenuation(win_type type, double beta)
{
    switch (type) {
    case (WIN_HAMMING):
        return 53;
        break;
    case (WIN_HANN):
        return 44;
        break;
    case (WIN_BLACKMAN):
        return 74;
        break;
    case (WIN_RECTANGULAR):
        return 21;
        break;
    case (WIN_KAISER):
        return (beta / 0.1102 + 8.7);
        break;
    case (WIN_BLACKMAN_hARRIS):
        return 92;
        break;
    case (WIN_BARTLETT):
        return 27;
        break;
    case (WIN_FLATTOP):
        return 93;
        break;
    default:
        throw std::out_of_range("window::max_attenuation: unknown window type provided.");
    }
}


std::vector<float> firfilter::build(win_type type, int ntaps)
{
    switch (type) {
    case WIN_HAMMING:
        return hamming(ntaps);
    case WIN_HANN:
        return hann(ntaps);
    case WIN_BLACKMAN:
        return blackman(ntaps);
    case WIN_BLACKMAN_hARRIS:
        return blackman_harris(ntaps);
    default:
        throw std::out_of_range("window::build: type out of range");
    }
}

std::vector<float> firfilter::blackman_harris(int ntaps, int atten)
{
    switch (atten) {
    case (61):
        return coswindow(ntaps, 0.42323, 0.49755, 0.07922);
    case (67):
        return coswindow(ntaps, 0.44959, 0.49364, 0.05677);
    case (74):
        return coswindow(ntaps, 0.40271, 0.49703, 0.09392, 0.00183);
    case (92):
        return coswindow(ntaps, 0.35875, 0.48829, 0.14128, 0.01168);
    default:
        throw std::out_of_range("window::blackman_harris: unknown attenuation value "
                                "(must be 61, 67, 74, or 92)");
    }
}

std::vector<float> firfilter::blackman(int ntaps)
{
    return coswindow(ntaps, 0.42, 0.5, 0.08);
}

std::vector<float> firfilter::hamming(int ntaps)
{
    std::vector<float> taps(ntaps);
    float M = static_cast<float>(ntaps - 1);

    for (int n = 0; n < ntaps; n++)
        taps[n] = 0.54 - 0.46 * cos((2 * M_PI * n) / M);
    return taps;
}

std::vector<float> firfilter::hann(int ntaps)
{
    std::vector<float> taps(ntaps);
    float M = static_cast<float>(ntaps - 1);

    for (int n = 0; n < ntaps; n++)
        taps[n] = 0.5 - 0.5 * cos((2 * M_PI * n) / M);
    return taps;
}

std::vector<float> firfilter::coswindow(int ntaps, float c0, float c1, float c2)
{
    std::vector<float> taps(ntaps);
    float M = static_cast<float>(ntaps - 1);

    for (int n = 0; n < ntaps; n++)
        taps[n] = c0 - c1 * cosf((2.0f * M_PI * n) / M) +
                  c2 * cosf((4.0f *M_PI * n) / M);
    return taps;
}

std::vector<float> firfilter::coswindow(int ntaps, float c0, float c1, float c2, float c3)
{
    std::vector<float> taps(ntaps);
    float M = static_cast<float>(ntaps - 1);

    for (int n = 0; n < ntaps; n++)
        taps[n] = c0 - c1 * cosf((2.0f * M_PI * n) / M) +
                  c2 * cosf((4.0f * M_PI * n) / M) -
                  c3 * cosf((6.0f * M_PI * n) / M);
    return taps;
}
