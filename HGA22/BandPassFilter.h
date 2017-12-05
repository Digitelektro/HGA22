#include <dsp.h>

#define XMEMORY __attribute__((space(xmemory),eds))
#define YMEMORY __attribute__((space(ymemory),eds))

extern FIRStruct BandPassFilter;

int Fir(int InSample);
void InitBandPassFilter();
