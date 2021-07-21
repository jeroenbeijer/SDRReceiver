#ifndef FIRFILTER_H
#define FIRFILTER_H

#include "qvector.h"

class firfilter
{
public:

    enum win_type {
        WIN_NONE = -1,       //!< don't use a window
        WIN_HAMMING = 0,     //!< Hamming window; max attenuation 53 dB
        WIN_HANN = 1,        //!< Hann window; max attenuation 44 dB
        WIN_BLACKMAN = 2,    //!< Blackman window; max attenuation 74 dB
        WIN_RECTANGULAR = 3, //!< Basic rectangular window
        WIN_KAISER = 4, //!< Kaiser window; max attenuation a function of beta, google it
        WIN_BLACKMAN_hARRIS = 5, //!< Blackman-harris window
        WIN_BLACKMAN_HARRIS =
        5,            //!< alias to WIN_BLACKMAN_hARRIS for capitalization consistency
        WIN_BARTLETT = 6, //!< Barlett (triangular) window
        WIN_FLATTOP = 7,  //!< flat top window; useful in FFTs
    };

    firfilter();
    double filter(double in);
    void setTaps(QVector<double> taps);
    ~firfilter();

    QVector<float>low_pass(double gain, double sampling_freq, double cutoff_freq, double transition_width, win_type window_type,double beta);

private:

    int M; // The number of taps, the length of the filter
    double Fc = 0x0D; // Will be set to cutoffFreq/SAMPLE_RATE;

    QVector<double> xv;  // This array holds the delayed values
    QVector<double> coeffs;

    std::vector<float> window(win_type type, int ntaps);
    void sanity_check_1f(double sampling_freq, double fa, double transition_width);
    int compute_ntaps(double sampling_freq,double transition_width, win_type window_type,double beta);

    double max_attenuation(win_type type, double beta);

    std::vector<float> build(win_type type, int ntaps);
    std::vector<float> blackman_harris(int ntaps, int atten = 92);
    std::vector<float> blackman(int ntaps);
    std::vector<float> hamming(int ntaps);
    std::vector<float> hann(int ntaps);
    std::vector<float> coswindow(int ntaps, float c0, float c1, float c2);
    std::vector<float> coswindow(int ntaps, float c0, float c1, float c2, float c3);


};

#endif // FIRFILTER_H
