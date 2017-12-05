#include <math.h>
#include <dsp.h>
#include <string.h>
#include"p33Exxxx.h"
#include "BandPassFilter.h"
#include "iq.h"

/*----------------------------------------------------------*/
/* Defines								               		*/
/*----------------------------------------------------------*/
#define DCC_PIN LATBbits.LATB0			//DCC input PIN
#define FCY 70000000LL           		//CPU Clock
#include <libpic30.h>            		//__delay_ms library, it must be under the FCY define
#define ADC_SAMPLERATE 20000
#define UART_BAUDRATE 9600
#define TMR3_PR FCY/ADC_SAMPLERATE
#define DMABUFFERSIZE 500


_FGS(GWRP_OFF & GCP_OFF )										//General Segment may be written, General Segment Code protect is Disabled
_FICD(ICS_PGD1 & JTAGEN_OFF)									//PGD1/PGC1 is debug pin, JTAG OFF,
_FWDT(FWDTEN_OFF)												//Watchdog off, Watchdog timer enabled/disabled by user software
_FOSCSEL(FNOSC_FRC & IESO_OFF);									//Select Internal FRC at POR
_FOSC(FCKSM_CSECMD & OSCIOFNC_OFF & POSCMD_XT & IOL1WAY_OFF); 	//Config for 4Mhz chrystal osc  
	

/*----------------------------------------------------------*/
/* Function Declarations				               		*/
/*----------------------------------------------------------*/
void DeviceInit();


/*----------------------------------------------------------*/
/* Variable Declarations				               		*/
/*----------------------------------------------------------*/
__eds__  unsigned int DMABufferA[DMABUFFERSIZE] __attribute__((eds));
__eds__  unsigned int DMABufferB[DMABUFFERSIZE] __attribute__((eds));
char UARTDMABuffer[DMABUFFERSIZE * sizeof(int)] __attribute__((eds));
int samplesBuffer[DMABUFFERSIZE] YMEMORY;
int filteredSamples[DMABUFFERSIZE] YMEMORY;
char startProcess = 0;
char bufferSwitch = 0;


/*----------------------------------------------------------*/
/* Main routine							               		*/
/*----------------------------------------------------------*/

int main()
{
    DeviceInit();		
	InitBandPassFilter();
	InitIQDemodulator();
	while(1)			
    {
		if(startProcess == 1)
		{
			startProcess = 0;
			FIR(DMABUFFERSIZE, filteredSamples, samplesBuffer, &BandPassFilter);
			AddSamples(filteredSamples, DMABUFFERSIZE);
		}
		
    }
	return 0;
}


/*----------------------------------------------------------*/
/* Device Init							               		*/
/*----------------------------------------------------------*/

void DeviceInit()
{
	//Configure PLL prescaler, PLL postscaler, PLL divisor
	CLKDIVbits.PLLPOST= 0; 	// N1=2 = 2MHz
	PLLFBD=138; //140		//280.00MHz
	CLKDIVbits.PLLPRE= 0; 	//2 = 140,00 ==70,00MHz FCY
	CLKDIVbits.DOZEN = 0;

	// Initiate Clock Switch to FRC oscillator with PLL (NOSC=0b001)
	__builtin_write_OSCCONH(0x03);
	__builtin_write_OSCCONL(OSCCON | 0x01);
	// Wait for Clock switch to occur
	while (OSCCONbits.COSC!= 0b011);
	// Wait for PLL to lock
	while (OSCCONbits.LOCK!= 1);

	//Port settings
   	ANSELA = 0;         	//Disable analog inputs
	ANSELB = 0;
    TRISB = 0;				//All port are outputs
    TRISA = 0;
	INTCON2bits.GIE = 1;	//Global interrupt enable


	//OC
	OC1CON1 = 0;
	OC1CON1bits.OCTSEL = 0b111;		//FCY is the clock source
	OC1CON1bits.OCM = 0b110;		//Edge-Aligned PWM mode
	OC1CON2bits.SYNCSEL = 0b11111;	//no sync
	OC1R = 270;		
	OC1RS = 535; 
	RPOR2bits.RP38R = 0b010000;		//OC Output pin is PORTB6


	//Timer2
	T2CON = 0;
	PR2 = 18000;
	T2CONbits.TCKPS = 0b01;
	IPC1bits.T2IP = 0b11; 			// Set Timer2 Interrupt Priority Level
	IFS0bits.T2IF = 0; 				// Clear Timer1 Interrupt Flag
	IEC0bits.T2IE = 1; 				// Enable Timer2 interrupt
	

	//UART
	RPOR2bits.RP39R = 0b000001;		//UART TX PORTB0
	TRISBbits.TRISB5 = 0;
	LATBbits.LATB5 = 1;
	RPINR18 = 0b0100101;			//UART RX PORTB.RB5
	U1BRG = ((FCY / 16) / UART_BAUDRATE) - 1;
	U1STA = 0x0000;
	U1MODE = 0x8000;
	U1STAbits.UTXEN = 1;			//TX Enable
	IFS0bits.U1TXIF = 0;
	IEC0bits.U1TXIE = 1;
	
	//ADC
	ANSELAbits.ANSA0= 1;		//AN0 Analog pin
	TRISAbits.TRISA0 = 1;		//AN0 input
	AD1CON1 = 0x14E4;			// DMA Conversion Order, sequential sampling, 12-bit, unsigned integer
	AD1CON1bits.ASAM = 1;
	AD1CON1bits.SSRCG = 0;
	AD1CON1bits.SSRC = 0b010;	//Timer3 start sampling
	AD1CON2 = 0x0000;		 
	AD1CON3bits.ADRC = 0;		//ADC Clock is derived from System Clock
	AD1CON3bits.SAMC = 2;	
	AD1CON3bits.ADCS = 199;		// TAD = TCY * (ADCS + 1) = 70000000 / (199 + 1)
								// ADC conversion time for 12-bit Tconv = 14 * Tad = 25000kHz
	AD1CON4 = 0x0100;			// Use DMA to store conversion results
	AD1CSSH = 0x0000;
	AD1CSSL = 0x0000;

	//DMA FOR ADC
	DMA3CON = 0;
	DMA3CONbits.AMODE = 0;						//Indirect mode with post increment
	DMA3CONbits.MODE = 0b10;					//Continous mode, ping pong buffer enabled
	DMA3CONbits.DIR = 0;						//Peripheral to RAM
	DMA3CONbits.SIZE = 0;						//word mode
	DMA3PAD = (volatile unsigned int)&ADC1BUF0;	//DMA point to ADC
	DMA3CNT = DMABUFFERSIZE - 1;				//BUFFER Size
	DMA3REQ = 13;								//ADC1 as DMA request source
	DMA3STAL = __builtin_dmaoffset(DMABufferA);
	DMA3STAH = 0x0000;
	DMA3STBL = __builtin_dmaoffset(DMABufferB);
	DMA3STBH = 0x0000;
	IFS2bits.DMA3IF = 0;
	IEC2bits.DMA3IE = 1;						//Enable interrupt
	DMA3CONbits.CHEN = 1;						//Enable DMA

	//TIMER3 For ADC 
	PR3 = TMR3_PR-1;
	T3CON = 0x8000;

	//Enable ADC module and provide ADC stabilization delay 
	AD1CON1bits.ADON = 1;
	__delay_us(20);

}





/*----------------------------------------------------------*/
/* Interrupts							               		*/
/*----------------------------------------------------------*/

void __attribute__((__interrupt__, no_auto_psv)) _U1TXInterrupt(void)
{
	IFS0bits.U1TXIF = 0; 
}

void __attribute__((__interrupt__, no_auto_psv)) _T1Interrupt(void)
{
	IFS0bits.T1IF = 0; 
}

void __attribute__((__interrupt__, no_auto_psv)) _T2Interrupt(void)
{
	IFS0bits.T2IF = 0; 
}

void __attribute__((interrupt, auto_psv)) _DMA0Interrupt(void)
{
	IFS0bits.DMA0IF = 0;
}

void __attribute__((interrupt, auto_psv)) _DMA3Interrupt(void)
{
	static int i;
	if(!bufferSwitch)
	{
		bufferSwitch = 1; 
		for(i = 0; i < DMABUFFERSIZE; i++)
		{
			samplesBuffer[i] = (DMABufferA[i] - 2048); 
		}
	}
	else
	{
		for(i = 0; i < DMABUFFERSIZE; i++)
		{
			samplesBuffer[i] = (DMABufferB[i] - 2048);
		}
		bufferSwitch = 0;
	}
	startProcess = 1;
	_DMA3IF = 0;	// Clear DMA interrupt flag to prepare for next block
}



/*---------------------------------------------------------*/
/* Error interrupts							               */
/*---------------------------------------------------------*/


void __attribute__((interrupt, no_auto_psv)) _OscillatorFail(void)
{
        INTCON1bits.OSCFAIL = 0;      //Clear the trap flag
        while (1);
}

void __attribute__((interrupt, no_auto_psv)) _AddressError(void)
{
		//errLoc = getErrLoc();		// get the location of error function
        INTCON1bits.ADDRERR = 0;	//Clear the trap flag
}
void __attribute__((interrupt, no_auto_psv)) _StackError(void)
{
        INTCON1bits.STKERR = 0; 	//Clear the trap flag
		while (1);
}

void __attribute__((interrupt, no_auto_psv)) _MathError(void)
{
        INTCON1bits.MATHERR = 0;    //Clear the trap flag
        while (1);
}