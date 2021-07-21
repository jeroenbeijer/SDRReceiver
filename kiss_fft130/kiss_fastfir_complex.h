#ifndef KISS_FASTFIR_COMPLEX_H
#define KISS_FASTFIR_COMPLEX_H
//moved header stuff from kiss_fastfir.c to here. crazy why it wasnt here in the first place
//jonti 2015
//
//made this complex fir so we can have both complex and real fast fir without having to choose one
//jonti 2017

#include "_kiss_fft_guts.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MIN_FFT_LEN_COMPLEX 1024

typedef struct kiss_fastfir_state_complex *kiss_fastfir_cfg_complex;

kiss_fastfir_cfg_complex kiss_fastfir_alloc_complex(const kiss_fft_cpx * imp_resp,size_t n_imp_resp,
        size_t * nfft,void * mem,size_t*lenmem);

/* see do_file_filter for usage */
size_t kiss_fastfir_complex( kiss_fastfir_cfg_complex cfg, kiss_fft_cpx * inbuf, kiss_fft_cpx * outbuf, size_t n, size_t *offset);


struct kiss_fastfir_state_complex{
    size_t nfft;
    size_t ngood;
    kiss_fft_cfg fftcfg;
    kiss_fft_cfg ifftcfg;
    kiss_fft_cpx * fir_freq_resp;
    kiss_fft_cpx * freqbuf;
    size_t n_freq_bins;
    kiss_fft_cpx * tmpbuf;
};


#ifdef __cplusplus
}
#endif
#endif  // KISS_FASTFIR_COMPLEX_H
