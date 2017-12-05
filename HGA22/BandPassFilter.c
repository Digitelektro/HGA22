#include "BandPassFilter.h"

/*

FIR filter designed with
 http://t-filter.appspot.com

sampling frequency: 20000 Hz

fixed point precision: 16 bits

* 0 Hz - 4600 Hz
  gain = 0
  desired attenuation = -40 dB
  actual attenuation = n/a

* 4800 Hz - 5200 Hz
  gain = 1
  desired ripple = 5 dB
  actual ripple = n/a

* 5400 Hz - 10000 Hz
  gain = 0
  desired attenuation = -40 dB
  actual attenuation = n/a

*/
#define FILTER_TAP 129

fractional firHistory[FILTER_TAP] YMEMORY;
FIRStruct BandPassFilter;

fractional coeffs[FILTER_TAP] XMEMORY= {
  -192,
  0,
  79,
  0,
  -91,
  0,
  101,
  0,
  -108,
  0,
  111,
  0,
  -108,
  0,
  99,
  0,
  -82,
  0,
  57,
  0,
  -22,
  0,
  -22,
  0,
  77,
  0,
  -142,
  0,
  218,
  0,
  -304,
  0,
  400,
  0,
  -503,
  0,
  615,
  0,
  -731,
  0,
  852,
  0,
  -975,
  0,
  1097,
  0,
  -1217,
  0,
  1332,
  0,
  -1440,
  0,
  1539,
  0,
  -1627,
  0,
  1701,
  0,
  -1761,
  0,
  1804,
  0,
  -1831,
  0,
  1840,
  0,
  -1831,
  0,
  1804,
  0,
  -1761,
  0,
  1701,
  0,
  -1627,
  0,
  1539,
  0,
  -1440,
  0,
  1332,
  0,
  -1217,
  0,
  1097,
  0,
  -975,
  0,
  852,
  0,
  -731,
  0,
  615,
  0,
  -503,
  0,
  400,
  0,
  -304,
  0,
  218,
  0,
  -142,
  0,
  77,
  0,
  -22,
  0,
  -22,
  0,
  57,
  0,
  -82,
  0,
  99,
  0,
  -108,
  0,
  111,
  0,
  -108,
  0,
  101,
  0,
  -91,
  0,
  79,
  0,
  -192
};

void InitBandPassFilter()
{
	FIRStructInit(&BandPassFilter, FILTER_TAP, coeffs, COEFFS_IN_DATA, firHistory);
}
