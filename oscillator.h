#ifndef OSCILLATOR_H
#define OSCILLATOR_H
#include <complex>

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

typedef std::complex<float> cpx_typef;


class Oscillator
{

private:
      cpx_typef _rotation;
      double _sampleRate;
      double _frequency;

      cpx_typef * queue;
      int queuePtr;
      int length;

public:
      Oscillator(double sampleRate, double Frequency);
      void tick();
      cpx_typef _vector;
      ~Oscillator();
};

#endif // OSCILLATOR_H
