// Alexander Wong - U94390492
// Kanav Dhir - U28185259
// EC 450 Final Project - Doorway counter

// This program uses a variety of sensors to detect how many people have moved through a doorway.
// As such, the sensor array should be position in the inside of a doorway.
// The total number of people passed/people in a room is displayed on a seven segment display.

#include "msp430g2553.h"
#include "math.h"

// Define bit masks for ADC pin and channel used as P1.4
#define ADC_INPUT_BIT_MASK 0x10		// Assign input pin to P1.4
#define ADC_INCH INCH_4				// Input channel
#define TA0_BIT 0x02				// Assign output pin to P1.1


 // Function prototypes
void init_adc(void);
void init_wdt(void);

// =======ADC Initialization and Interrupt Handler========

// Global variables that store the results (read from the debugger)
volatile int latest_result;   // most recent result is stored in latest_result
volatile int count = 0;
volatile int ambient;
int first_time = 1;
int lower = 0;


// Initialization of the ADC
void init_adc(){
	ADC10CTL1= ADC_INCH				// input channel 4
			  +SHS_0				// use ADC10SC bit to trigger sampling
			  +ADC10DIV_4			// ADC10 clock/5
			  +ADC10SSEL_0			// Clock Source=ADC10OSC
			  +CONSEQ_0;			// single channel, single conversion
			  ;
	ADC10AE0=ADC_INPUT_BIT_MASK;	// enable A4 analog input
	ADC10CTL0= SREF_0				// reference voltages are Vss and Vcc
			  +ADC10SHT_3			// 64 ADC10 Clocks for sample and hold time (slowest)
			  +ADC10ON				// turn on ADC10
			  +ENC					// enable (but not yet start) conversions
			  +ADC10IE				// enable interrupts
			  ;
}


void init_wdt(){
	// setup the watchdog timer as an interval timer
	WDTCTL =(WDTPW +		// (bits 15-8) password
							// bit 7=0 => watchdog timer on
							// bit 6=0 => NMI on rising edge (not used here)
							// bit 5=0 => RST/NMI pin does a reset (not used here)
			 WDTTMSEL +     // (bit 4) select interval timer mode
			 WDTCNTCL  		// (bit 3) clear watchdog timer counter
							// bit 2=0 => SMCLK is the source
					+ 1		// bits 1-0 = 00 => source/8K
			 );
	 IE1 |= WDTIE;			// enable the WDT interrupt (in the system interrupt register IE1)
}

// Main initializes everything and leaves the CPU in low power mode.
void main(){

	WDTCTL = WDTPW + WDTHOLD;       // Stop watchdog timer
	BCSCTL1 = CALBC1_8MHZ;			// 8Mhz calibration for clock
  	DCOCTL  = CALDCO_8MHZ;

  	init_adc();
  	init_wdt();

  	ADC10CTL0 |= ADC10SC;			// Trigger one conversion

	_bis_SR_register(GIE+LPM0_bits);
}


// *****Interrupt Handlers*****

// ADC10 Interrupt Handler
void interrupt adc_handler(){ // Invoked when a conversion is complete
	latest_result = ADC10MEM;			// store the answer in memory

	if (first_time){
		ambient  = latest_result;
		first_time = 0;
	}

}
ISR_VECTOR(adc_handler, ".int05")

// Watchdog Timer Interrupt Handler
interrupt void WDT_interval_handler(){
	/*
	if (interval-- == 0){
		ADC10CTL0 |= ADC10SC;
		previous_result = latest_result;
	}
	else{
		previous_result = latest_result;
	}
	if (abs(previous_result - latest_result) > 10){
		count++;
		interval = 200;
	}
	*/

	if (ambient - latest_result > 10){ // Detect downward edge
		lower = 1;
	}
	else if (lower == 0){
		ambient = (ambient+latest_result)/2;		// Average ambient light
	}
	else if (lower == 1){
		if (abs(ambient - latest_result) < 10){
			count++;
			lower = 0;
		}
	}


	ADC10CTL0 |= ADC10SC; // trigger a conversion
}
ISR_VECTOR(WDT_interval_handler, ".int10")
