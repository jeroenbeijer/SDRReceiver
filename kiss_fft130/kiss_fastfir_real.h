#ifndef KISS_FASTFIR_REAL_H
#define KISS_FASTFIR_REAL_H
//moved header stuff from kiss_fastfir.c to here. crazy why it wasnt here in the first place
//jonti 2015
//
//made this real fir so we can have both complex and real fast fir without having to choose one
//jonti 2017

#include "_kiss_fft_guts.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MIN_FFT_LEN_REAL 2048
#include "kiss_fftr.h"

typedef struct kiss_fastfir_state_real *kiss_fastfir_cfg_real;

kiss_fastfir_cfg_real kiss_fastfir_alloc_real(const kiss_fft_scalar * imp_resp,size_t n_imp_resp,
        size_t * nfft,void * mem,size_t*lenmem);

/* see do_file_filter for usage */
size_t kiss_fastfir_real( kiss_fastfir_cfg_real cfg, kiss_fft_scalar * inbuf, kiss_fft_scalar * outbuf, size_t n, size_t *offset);


struct kiss_fastfir_state_real{
    size_t nfft;
    size_t ngood;
    kiss_fftr_cfg fftcfg;
    kiss_fftr_cfg ifftcfg;
    kiss_fft_cpx * fir_freq_resp;
    kiss_fft_cpx * freqbuf;
    size_t n_freq_bins;
    kiss_fft_scalar * tmpbuf;
};


#ifdef __cplusplus
}
#endif
#endif  // KISS_FASTFIR_REAL_H
