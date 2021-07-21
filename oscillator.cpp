#include <math.h>
#include "oscillator.h"

Oscillator::Oscillator(double sampleRate, double Frequency)
{

    _frequency = Frequency;
    _sampleRate = sampleRate;
    double anglePerSample = 2.0 * M_PI * _frequency / _sampleRate;

    _rotation = cpx_typef((float)cos(anglePerSample), (float)sin(anglePerSample));

    _vector = cpx_typef(1.0f,0);

    length = (int)sampleRate;


    queue = new cpx_typef[length];

    for(int i=0;i<length;i++)
    {
        _vector *= _rotation;
         float norm = 1.95f - (_vector.real()*_vector.real()+_vector.imag()*_vector.imag());
        _vector = _vector * norm;

        queue[i] = _vector;

    }

    queuePtr = 0;

}

Oscillator::~Oscillator()
{

    delete queue;
}
void Oscillator::tick()
{

     queuePtr++;
     if(queuePtr == length)
     {
         queuePtr = 0;
     }

     _vector=queue[queuePtr];

}

