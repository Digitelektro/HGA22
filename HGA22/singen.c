#include "singen.h"
#include <math.h>


#define PI 3.14159265358979323846f
#define TABLESIZE 256
#define SAMPLERATE 20000
#define FREQUENCY 4950

/*----------------------------------------------------------*/
/* Variable Declarations				               		*/
/*----------------------------------------------------------*/
int16_t SinTable[TABLESIZE + 1];
int16_t CosTable[TABLESIZE + 1];
uint32_t PhaseIncrement;
uint32_t PhaseAccumulator = 0;
uint32_t CosPhaseAccumulator = 0;
uint32_t Fractional = 0;
uint32_t Index;
int16_t v1;
int16_t v2;
uint32_t result;


/*----------------------------------------------------------*/
/* Functions							               		*/
/*----------------------------------------------------------*/
void InitSinWave()
{
    int i;
    for (i = 0; i < TABLESIZE + 1; i++)
    {
        SinTable[i] = (int16_t)(511.0f * sin(2.0f * PI * i / TABLESIZE));
		CosTable[i] = (int16_t)(511.0f * cos(2.0f * PI * i / TABLESIZE));
    }
    PhaseIncrement = 65536LL * TABLESIZE * FREQUENCY / SAMPLERATE; 
}

uint16_t GetSinNextSample()
{
    PhaseAccumulator+=PhaseIncrement;
    PhaseAccumulator &= 0xFFFFFF; //Limit phase acc to 24 bit
    Index = PhaseAccumulator >> 16;
    Fractional = PhaseAccumulator & 0xFFFF;
    v1 = SinTable[Index];
    v2 = SinTable[Index + 1];
    result = v1 + (v2 - v1);
    v1 = 0;
    return (uint16_t)(result);
}

uint16_t GetCosNextSample()
{
    CosPhaseAccumulator+=PhaseIncrement;
    CosPhaseAccumulator &= 0xFFFFFF; //Limit phase acc to 24 bit
    Index = CosPhaseAccumulator >> 16;
    Fractional = CosPhaseAccumulator & 0xFFFF;
    v1 = CosTable[Index];
    v2 = CosTable[Index + 1];
    result = v1 + (v2 - v1);
    v1 = 0;
    return (uint16_t)(result);
}