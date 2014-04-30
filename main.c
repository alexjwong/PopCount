// Alexander Wong - U94390492
// Kanav Dhir - U28185259
// EC 450 Final Project - Doorway counter

// This program currently uses photoresistors to detect how many people have moved through a doorway.
// As such, the sensors should be position in the inside of a doorway, preferably with ample light.
// The total number of people passed/people in a room is displayed on a LCD display.

#include "msp430g2553.h"
#include "math.h"
#include "lcd16.h"

// Define bit masks for ADC pin and channel used as P1.4
#define RIGHT_SENSOR_MASK 0x20				// Assign input pin to P1.5
#define LEFT_SENSOR_MASK 0x10				// Assign input pin to P1.4
#define RIGHT_SENSOR INCH_5					// Assign input pin to P1.5
#define LEFT_SENSOR INCH_4					// Assign input pin to P1.4
#define SENSITIVITY 8
#define BUTTON BIT4							// P2.4

 // Function prototypes
void init_adc(void);
void init_wdt(void);
void init_gpio(void);

// Global variables that store the results (read from the debugger)
volatile int latest_result_left;	// most recent result is stored in x_latest_result
volatile int latest_result_right;
volatile int count = 0;				// Holds overall person count
volatile int attendance = 0;
volatile int ambient_left = 0;		// Ambient light level for left sensor
volatile int ambient_right = 0;		// Ambient light level for right sensor
int first_time_left = 1;			// BOOL to check if first time after start up
int first_time_right = 1;			// BOOL to check if first time after start up
int lower_left = 0;					// BOOL
int lower_right = 0;				// BOOL
int trigger_left = 0;				// BOOL
int trigger_right = 0;				// BOOL
int timeout = 0;					// BOOL timeout
int timeout_period = 0;
int state = 0;						// Tracks which mode the program is in

void init_adc() {
	ADC10CTL0 = ADC10ON			// turn on ADC
				+ADC10SHT_1		// spend 8 clock cycles to take a sample
				+SREF_0			// V_r+ = Vcc, V_r- = ground
				+ADC10IE		// enable ADC interrupt
				;
	ADC10CTL1 = ADC10SSEL_0;	// +clock select, invert sample input
}

void get_left_sensor() {
	ADC10CTL1 &= ~RIGHT_SENSOR;		// select input channel
	ADC10CTL1 |= LEFT_SENSOR;
	ADC10AE0 = LEFT_SENSOR_MASK;	// enable analog input of left speaker
	ADC10CTL0 |= ENC + ADC10SC;		// enable conversion, start sample-and-conversion

	while(1){
		if (((ADC10CTL0 & ADC10IFG)==ADC10IFG)) {
			ADC10CTL0 &= ~(ADC10IFG +ENC);
			break;
		}
	}
	latest_result_left = ADC10MEM;
	ADC10CTL0 &= ~ENC;

	if (first_time_left){
		ambient_left  = latest_result_left;
		first_time_left = 0;
	}
}

void get_right_sensor() {
	ADC10CTL1 &= ~LEFT_SENSOR;		// select input channel
	ADC10CTL1 |= RIGHT_SENSOR;
	ADC10AE0 = RIGHT_SENSOR_MASK;
	ADC10CTL0 |= ENC + ADC10SC;		// enable conversion, start sample-and-conversion

	while(1){
		if (((ADC10CTL0 & ADC10IFG)==ADC10IFG)) {
			ADC10CTL0 &= ~(ADC10IFG +ENC);
			break;
		}
	}
	latest_result_right = ADC10MEM;
	ADC10CTL0 &= ~ENC;

	if (first_time_right){
		ambient_right  = latest_result_right;
		first_time_right = 0;
	}
}
void init_gpio(){	// Init for Button Interrupt method - *isn't working*
	P2DIR &= ~BUTTON;		// Sets the button to input
	P2OUT |= BUTTON;		// pullup on input pin
	P2REN &= ~BUTTON;		// Resistor Enable
	P2IE |= BUTTON;			// enable interrupts on button pin
	P2IES |= BUTTON;		// set to interrupt on down going edge
	P2IFG &= ~BUTTON;		// clear interrupt flag
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

// ***Main initializes everything and leaves the CPU in low power mode.***
void main(){

	BCSCTL1 = CALBC1_8MHZ;			// 8Mhz calibration for clock
  	DCOCTL  = CALDCO_8MHZ;

  	// Initialize P1 for LCD and two input pins for ADC10
	P1DIR = 0xCF;
	P1OUT = 0x00;

  	init_adc();
  	init_wdt();
	lcd_init();
	//init_gpio();					// Initialization of GPIO for interrupt method - CURRENTLY DOES NOT WORK!
	P2REN |= BUTTON;				// Initialization of GPIO for POLLING method.

	ADC10CTL0 |= ENC + ADC10SC;		// Trigger one conversion for initial ambient values

  	prints("Population:");

	_bis_SR_register(GIE+LPM0_bits);
}

// *****Interrupt Handlers*****

// ADC10 Interrupt Handler
void interrupt adc_handler(){ // Invoked when a conversion is complete
	ADC10CTL0 &= ~ADC10IFG;  // clear interrupt flag
}
ISR_VECTOR(adc_handler, ".int05")


interrupt void button_handler(){
	switch(state){
		case 0:{
			state = 1;
			gotoXy(0,0);
			prints("Attendance:");
		}
		case 1:{
			state = 0;
			gotoXy(0,0);
			prints("Population:");
		}
	}
	P2IFG &= ~BUTTON;									// Clear interrupt flag
}
ISR_VECTOR(button_handler, ".int03")

// Watchdog Timer Interrupt Handler
interrupt void WDT_interval_handler(){
	// Polling state machine for button
	if (P2IN == 0){
		while (1){
			if (state == 0 && P2IN == BUTTON){
				state = 1;
				gotoXy(0,0);
				prints("Attendance:");
				break;
			}
			else if (state == 1 & P2IN == BUTTON){
				state = 0;
				gotoXy(0,0);
				prints("Population:");
				break;
			}
		}
	}

	// Action time out counter
	if (timeout == 1){
		gotoXy(10,1);
		prints("Wait");
		if (timeout_period >= 500){
			timeout_period = 0;
			timeout = 0;
			trigger_left = 0;
			trigger_right = 0;
		}
		else
			timeout_period++;
	}
	else if (timeout == 0){
		timeout_period = 0;
		gotoXy(10,1);
		prints("    ");
	}

	// Initialize readings for both sensors
	get_right_sensor();
	get_left_sensor();

	// Sensors work by detecting a downward edge in the light readings, and then waiting for the light level
	// to rise back to ambient.

	// You must pass by both sensors to implement a count
	// - each sensor triggers a check in the other to verify a pass.

	//Left----------------------------------------------------------------------------------------------------------
	if (ambient_left - latest_result_left > SENSITIVITY){	// Detect downward edge
		lower_left = 1;
	}
	else if (lower_left == 0){
		ambient_left = (ambient_left + latest_result_left)/2;		// Average ambient light
	}
	else if (lower_left == 1){
		if (abs(ambient_left - latest_result_left) < SENSITIVITY){
			if (trigger_right == 1 && timeout == 1){
				if(state == 0){	// STATE - if in population mode, will decrement
								//			  in attendance mode, will NOT decrement
					count--;
				}
				trigger_right = 0;
				lower_left = 0;
				timeout = 0;
			}
			else {
				trigger_left = 1;
				timeout = 1;
				lower_left = 0;
			}
		}
	}
	//Right----------------------------------------------------------------------------------------------------------
	if (ambient_right - latest_result_right > SENSITIVITY){	// Detect downward edge
		lower_right = 1;
	}
	else if (lower_right == 0){
		ambient_right = (ambient_right + latest_result_right)/2;		// Average ambient light
	}
	else if (lower_right == 1){
		if (abs(ambient_right - latest_result_right) < SENSITIVITY){
			if (trigger_left == 1 && timeout == 1){
				count++;
				attendance++;
				trigger_left = 0;
				lower_right = 0;
				timeout = 0;
			}
			else {
				trigger_right = 1;
				timeout = 1;
				lower_right = 0;
			}
		}
	}

	// Printing the current count to the LCD
	if (count < 0){
		/* - Code if you wanted to display negative counts
		gotoXy(0,1);
		prints("-");
		gotoXy(1,1);
		integerToLcd(abs(count));
		*/
		count = 0;
	}
	else {
		if (state == 0){
			gotoXy(1,1);
			integerToLcd(count);
		}
		else if (state == 1){
			gotoXy(1,1);
			integerToLcd(attendance);
		}

	}
}
ISR_VECTOR(WDT_interval_handler, ".int10")

