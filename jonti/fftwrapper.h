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

#ifndef FFTWRAPPER_H
#define FFTWRAPPER_H

#include <QVector>
#include <complex>
#include "kiss_fft130/kiss_fft.h"

//underlying fft still uses the type in the  kiss_fft_type in the c stuff
template<typename T>
class FFTWrapper
{
public:
    FFTWrapper(int nfft,bool inverse);
    ~FFTWrapper();
    void transform(const QVector< std::complex<T> > &in, QVector< std::complex<T> > &out);
private:
    kiss_fft_cfg cfg;
    QVector<kiss_fft_cpx> privatein;
    QVector<kiss_fft_cpx> privateout;
    int nfft;
};

#endif // FFTWRAPPER_H
