/*
Copyright (c) 2003-2004, Mark Borgerding

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    * Neither the author nor the names of any contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "kiss_fastfir_real.h"



//static int verbose=0;


kiss_fastfir_cfg_real kiss_fastfir_alloc_real(
        const kiss_fft_scalar * imp_resp,size_t n_imp_resp,
        size_t *pnfft, /* if <= 0, an appropriate size will be chosen */
        void * mem,size_t*lenmem)
{
    kiss_fastfir_cfg_real st = NULL;
    size_t len_fftcfg,len_ifftcfg;
    size_t memneeded = sizeof(struct kiss_fastfir_state_real);
    char * ptr;
    size_t i;
    size_t nfft=0;
    float scale;
    int n_freq_bins;
    if (pnfft)
        nfft=*pnfft;

    if (nfft<=0) {
        /* determine fft size as next power of two at least 2x 
         the impulse response length*/
        i=n_imp_resp-1;
        nfft=2;
        do{
             nfft<<=1;
        }while (i>>=1);
#ifdef MIN_FFT_LEN_REAL
        if ( nfft < MIN_FFT_LEN_REAL )
            nfft=MIN_FFT_LEN_REAL;
#endif        
    }
    if (pnfft)
        *pnfft = nfft;

    n_freq_bins = nfft/2 + 1;

    /*fftcfg*/
    kiss_fftr_alloc (nfft, 0, NULL, &len_fftcfg);
    memneeded += len_fftcfg;  
    /*ifftcfg*/
    kiss_fftr_alloc (nfft, 1, NULL, &len_ifftcfg);
    memneeded += len_ifftcfg;  
    /* tmpbuf */
    memneeded += sizeof(kiss_fft_scalar) * nfft;
    /* fir_freq_resp */
    memneeded += sizeof(kiss_fft_cpx) * n_freq_bins;
    /* freqbuf */
    memneeded += sizeof(kiss_fft_cpx) * n_freq_bins;
    
    if (lenmem == NULL) {
        st = (kiss_fastfir_cfg_real) malloc (memneeded);
    } else {
        if (*lenmem >= memneeded)
            st = (kiss_fastfir_cfg_real) mem;
        *lenmem = memneeded;
    }
    if (!st)
        return NULL;

    st->nfft = nfft;
    st->ngood = nfft - n_imp_resp + 1;
    st->n_freq_bins = n_freq_bins;
    ptr=(char*)(st+1);

    st->fftcfg = (kiss_fftr_cfg)ptr;
    ptr += len_fftcfg;

    st->ifftcfg = (kiss_fftr_cfg)ptr;
    ptr += len_ifftcfg;

    st->tmpbuf = (kiss_fft_scalar*)ptr;
    ptr += sizeof(kiss_fft_scalar) * nfft;

    st->freqbuf = (kiss_fft_cpx*)ptr;
    ptr += sizeof(kiss_fft_cpx) * n_freq_bins;
    
    st->fir_freq_resp = (kiss_fft_cpx*)ptr;
    ptr += sizeof(kiss_fft_cpx) * n_freq_bins;

    kiss_fftr_alloc (nfft,0,st->fftcfg , &len_fftcfg);
    kiss_fftr_alloc (nfft,1,st->ifftcfg , &len_ifftcfg);

    memset(st->tmpbuf,0,sizeof(kiss_fft_scalar)*nfft);
    /*zero pad in the middle to left-rotate the impulse response 
      This puts the scrap samples at the end of the inverse fft'd buffer */
    st->tmpbuf[0] = imp_resp[ n_imp_resp - 1 ];
    for (i=0;i<n_imp_resp - 1; ++i) {
        st->tmpbuf[ nfft - n_imp_resp + 1 + i ] = imp_resp[ i ];
    }

    kiss_fftr(st->fftcfg,st->tmpbuf,st->fir_freq_resp);

    /* TODO: this won't work for fixed point */
    scale = 1.0 / st->nfft;

    for ( i=0; i < st->n_freq_bins; ++i ) {
#ifdef USE_SIMD
        st->fir_freq_resp[i].r *= _mm_set1_ps(scale);
        st->fir_freq_resp[i].i *= _mm_set1_ps(scale);
#else
        st->fir_freq_resp[i].r *= scale;
        st->fir_freq_resp[i].i *= scale;
#endif
    }
    return st;
}

static void fastconv1buf_real(const kiss_fastfir_cfg_real st,const kiss_fft_scalar * in,kiss_fft_scalar * out)
{
    size_t i;
    /* multiply the frequency response of the input signal by
     that of the fir filter*/
    kiss_fftr( st->fftcfg, in , st->freqbuf );
    for ( i=0; i<st->n_freq_bins; ++i ) {
        kiss_fft_cpx tmpsamp; 
        C_MUL(tmpsamp,st->freqbuf[i],st->fir_freq_resp[i]);
        st->freqbuf[i] = tmpsamp;
    }

    /* perform the inverse fft*/
    kiss_fftri(st->ifftcfg,st->freqbuf,out);
}

/* n : the size of inbuf and outbuf in samples
   return value: the number of samples completely processed
   n-retval samples should be copied to the front of the next input buffer */
static size_t kff_nocopy_real(
        kiss_fastfir_cfg_real st,
        const kiss_fft_scalar * inbuf,
        kiss_fft_scalar * outbuf,
        size_t n)
{
    size_t norig=n;
    while (n >= st->nfft ) {
        fastconv1buf_real(st,inbuf,outbuf);
        inbuf += st->ngood;
        outbuf += st->ngood;
        n -= st->ngood;
    }
    return norig - n;
}

static
size_t kff_flush_real(kiss_fastfir_cfg_real st,const kiss_fft_scalar * inbuf,kiss_fft_scalar * outbuf,size_t n)
{
    size_t zpad=0,ntmp;

    ntmp = kff_nocopy_real(st,inbuf,outbuf,n);
    n -= ntmp;
    inbuf += ntmp;
    outbuf += ntmp;

    zpad = st->nfft - n;
    memset(st->tmpbuf,0,sizeof(kiss_fft_scalar)*st->nfft );
    memcpy(st->tmpbuf,inbuf,sizeof(kiss_fft_scalar)*n );
    
    fastconv1buf_real(st,st->tmpbuf,st->tmpbuf);
    
    memcpy(outbuf,st->tmpbuf,sizeof(kiss_fft_scalar)*( st->ngood - zpad ));
    return ntmp + st->ngood - zpad;
}

size_t kiss_fastfir_real(
        kiss_fastfir_cfg_real vst,
        kiss_fft_scalar * inbuf,
        kiss_fft_scalar * outbuf,
        size_t n_new,
        size_t *offset)
{
    size_t ntot = n_new + *offset;
    if (n_new==0) {
        return kff_flush_real(vst,inbuf,outbuf,ntot);
    }else{
        size_t nwritten = kff_nocopy_real(vst,inbuf,outbuf,ntot);
        *offset = ntot - nwritten;
        /*save the unused or underused samples at the front of the input buffer */
        if(nwritten>0)memcpy( inbuf , inbuf+nwritten , *offset * sizeof(kiss_fft_scalar) );
        return nwritten;
    }
}

