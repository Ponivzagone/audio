#ifndef FFT_H
#define FFT_H
#include <complex>

class FFT
{
public:
    FFT();
    void ForwardFft(std::complex<double> *, int);
    ~FFT();
};

#endif // FFT_H
