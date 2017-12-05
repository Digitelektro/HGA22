#include <dsp.h>
#include <string.h>
#include"p33Exxxx.h"
#include "IQCoeffs.h"
#include "singen.h"

#define FILTER_TAP 233
#define SAMPLERATE 20000
#define CARRIERFREQ 4950
#define STATECOUNT 4
#define BITFREQ 200
#define BITSPACE 25		//SampleRate / BitFreq / STATECOUNT
#define BUFFERLEN 2000  //BITSPACE* STATECOUNT * 10


void ExtractBits();

int iSample;
int qSample;
int filteredISamples[BUFFERLEN];
int filteredQSamples[BUFFERLEN];
int demodulatedSamples[BUFFERLEN];
int PrevIsample;
int PrevQsample;
int iqSamplesCounter;
int Sin;
int Cos;

fractional iFirHistory[FILTER_TAP] YMEMORY;
fractional qFirHistory[FILTER_TAP] YMEMORY;
FIRStruct iFilter;
FIRStruct qFilter;


char decodedData;
unsigned char startDetected;
unsigned char stopDetected;
unsigned char bitCounter;
unsigned char stopBit;
unsigned char parityBit;
unsigned char parityCounter;
unsigned char DataStarted;
unsigned char State;
unsigned char StateCounter;
float f;

void InitIQDemodulator()
{
	InitSinWave();
	FIRStructInit(&iFilter, FILTER_TAP, Coeffs, COEFFS_IN_DATA, iFirHistory);
	FIRStructInit(&qFilter, FILTER_TAP, Coeffs, COEFFS_IN_DATA, qFirHistory);
	iqSamplesCounter = 0;
}


void AddSamples(int* Samples, int len)
{
	int i;
	for (i = 0; i < len; i++)
	{
		Sin = GetSinNextSample();
		Cos = GetCosNextSample();
		//Create IQ data
		iSample = ((long)Sin *  Samples[i]) >> 5;
		qSample = ((long)Cos *  Samples[i]) >> 5;
		FIR(1,&filteredISamples[iqSamplesCounter], &iSample, &iFilter);
		FIR(1,&filteredQSamples[iqSamplesCounter], &qSample, &qFilter);
		iqSamplesCounter++;
		if(iqSamplesCounter == BUFFERLEN)
		{
			ExtractBits();
			iqSamplesCounter = 0;
		}
	}
}
void ExtractBits()
{
	int i;
	for(i = 0; i < BUFFERLEN; i++)
	{
		//Demodulation equation: (Q * (I-1) - I * (Q-1) / (I^2 * Q^2)
		f = (float)((long)filteredQSamples[i] * PrevIsample - (long)filteredISamples[i] * PrevQsample) / (float)((long)filteredQSamples[i] * filteredQSamples[i] + (long)filteredISamples[i] * filteredISamples[i]);
		demodulatedSamples[i] = (int)(f * 32767);
		PrevIsample = filteredISamples[i];
		PrevQsample = filteredQSamples[i];
	}
	//Get received data
	for(i = 0; i < BUFFERLEN; i+=BITSPACE)
	{
		if(demodulatedSamples[i] > 0)
		{
			DataStarted = 1;
			State++;
		}
		if(DataStarted)
		{
			StateCounter++;
		}
		if(StateCounter == STATECOUNT)
		{
			if(startDetected == 1)
			{
				if(bitCounter < 8)
				{
					if(State > 2)
					{
						
					}
					else
					{
						decodedData |= 1 << bitCounter;
					}
					bitCounter++;
				}
				else if(bitCounter == 8)
				{
					if(State > 2)
						parityBit = 0;
					else
						parityBit = 1;
					bitCounter++;
				}
				else if(bitCounter == 9)
				{
					if(State > 2)
						stopDetected = 0;
					else
						stopDetected = 1;
					if(stopDetected == 1)
					{
						unsigned char x;
						parityCounter = 0;
						for(x = 0; x < 8; x++)
						{
							if(decodedData & (1 << x))
								parityCounter++;
						}
						if(((parityCounter % 2) == parityBit))
						{
							U1TXREG = decodedData;
						}
					}
					bitCounter = 0;
					startDetected = 0;
					decodedData = 0;
					DataStarted = 0;			
				}		
				
			}
			else
			{
				if(State > 2)
				{
					startDetected = 1;
				}
			}
			State = 0;
			StateCounter = 0;
		}	
	}

}