/* The MIT License (MIT)

Copyright (c) 2015-2019 Jonathan Olds

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/

#include "fftrwrapper.h"
#include <assert.h>

template<class T>
FFTrWrapper<T>::FFTrWrapper(int _nfft,bool inverse)
{
    nfft=_nfft;
    cfg = kiss_fftr_alloc(nfft, inverse, NULL, NULL);
    privatescalar.resize(nfft);
    privatecomplex.resize(nfft);
}

template<class T>
FFTrWrapper<T>::~FFTrWrapper()
{
    free(cfg);

}

template<class T>
void FFTrWrapper<T>::transform(const QVector<T> &in, QVector< std::complex<T> > &out)
{
    assert(in.size()==out.size());
    assert(in.size()==nfft);
    for(int i=0;i<in.size();i++)
    {
        privatescalar[i]=in[i];
    }
    kiss_fftr(cfg,privatescalar.data(),privatecomplex.data());
    for(int i=0;i<out.size();i++)
    {
        out[i]=std::complex<T>((double)privatecomplex[i].r,(double)privatecomplex[i].i);
    }
}

template<class T>
void FFTrWrapper<T>::transform(const QVector< std::complex<T> > &in, QVector<T> &out)
{
    assert(in.size()==out.size());
    assert(in.size()==nfft);
    for(int i=0;i<in.size();i++)
    {
        privatecomplex[i].r=in[i].real();
        privatecomplex[i].i=in[i].imag();
    }
    kiss_fftri(cfg,privatecomplex.data(),privatescalar.data());
    for(int i=0;i<out.size();i++)
    {
        out[i]=privatescalar[i];
    }
}

template class FFTrWrapper<float>;
template class FFTrWrapper<double>;
