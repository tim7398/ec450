#include "msp430g2553.h"

//port declarations for SPI, LED, and Button
#define RED 0x01
#define BUTTON 0x08
#define SPI_CLK 0x20
#define SPI_SOMI 0x40
#define SPI_SIMO 0x80
#define BRLO (BIT_RATE_DIVISOR &  0xFF)
#define BRHI (BIT_RATE_DIVISOR / 0x100)
#define ACTION_INTERVAL 1
#define BIT_RATE_DIVISOR 32
#define BUTTON_BIT_PAUSE 0x01
#define BUTTON_BIT_ONE 0x02
#define BUTTON_BIT_TWO 0x04
#define BUTTON_BIT_THREE 0x08
#define BUTTON_BIT_FOUR 0x10

//Global Variables
volatile unsigned char last_button = 0;		//button tracker
volatile unsigned long randint = 0;
volatile unsigned long first_byte = 0;		//lower bits of the guess
volatile unsigned long second_byte = 0;		//upper bits of the guess
volatile unsigned long third_byte = 0;		//upper bits of the guess
volatile unsigned long fourth_byte = 0;		//upper bits of the guess
volatile unsigned long read_key = 0;		//combined guess
volatile unsigned int WDT_counter = 0;		//WDT counter
volatile unsigned int state = 's';			//state of the game
volatile unsigned int started = 0;			//whether the game is running
volatile unsigned long data_received = 0;	//last byte received
volatile unsigned int data_to_send = 0;		//character to be sent
volatile unsigned int tx_mode = 0;			//transmit mode
volatile unsigned int rx_mode = 0;			//receive mode
volatile unsigned int pin1 = 0;
volatile unsigned int pin2 = 0;
volatile unsigned int pin = 0;
volatile unsigned int typed_pin = 0;
volatile unsigned long key = 0xFFFF;
volatile unsigned int pin_index = 0;
volatile unsigned char debounce = 32;
volatile unsigned char changed = 0;
volatile unsigned char pin_digits [4] = {0,0,0,0};
volatile unsigned char digit1 = 0;
volatile unsigned char digit2 = 0;
volatile unsigned char digit3 = 0;
volatile unsigned char digit4 = 0;
volatile unsigned int lock_timeout = 0;

//function declarations
void init_button(void);
void init_spi(void);
void init_wdt(void);
unsigned int rand(void);

//main program
void main(){
	WDTCTL = WDTPW + WDTHOLD;	// Stop watchdog timer
	BCSCTL1 = CALBC1_1MHZ;
  	DCOCTL  = CALDCO_1MHZ;

  	P1DIR &= ~BUTTON;			//button initialization
  	P1OUT |= BUTTON;
  	P1REN |= BUTTON;

  	P1DIR |= RED;				//LED initialization
  	P1OUT &= ~RED;

  	pin = (0xFFFF & rand()) % 10000;

  	init_button();
  	init_spi();
  	init_wdt();
 	_bis_SR_register(GIE+LPM0_bits);
}

void init_button(){
// All GPIO's are already inputs if we are coming in after a reset
	P1OUT |= BUTTON_BIT_ONE; // pullup
	P1REN |= BUTTON_BIT_ONE; // enable resistor
	P1IES |= BUTTON_BIT_ONE; // set for 1->0 transition
	P1IFG &= ~BUTTON_BIT_ONE;// clear interrupt flag
	P1IE  |= BUTTON_BIT_ONE; // enable interrupt
	P1OUT |= BUTTON_BIT_TWO; // pullup
	P1REN |= BUTTON_BIT_TWO; // enable resistor
	P1IES |= BUTTON_BIT_TWO; // set for 1->0 transition
	P1IFG &= ~BUTTON_BIT_TWO;// clear interrupt flag
	P1IE  |= BUTTON_BIT_TWO; // enable interrupt
	P1OUT |= BUTTON_BIT_THREE; // pullup
	P1REN |= BUTTON_BIT_THREE; // enable resistor
	P1IES |= BUTTON_BIT_THREE; // set for 1->0 transition
	P1IFG &= ~BUTTON_BIT_THREE;// clear interrupt flag
	P1IE  |= BUTTON_BIT_THREE; // enable interrupt
	P1OUT |= BUTTON_BIT_FOUR; // pullup
	P1REN |= BUTTON_BIT_FOUR; // enable resistor
	P1IES |= BUTTON_BIT_FOUR; // set for 1->0 transition
	P1IFG &= ~BUTTON_BIT_FOUR;// clear interrupt flag
	P1IE  |= BUTTON_BIT_FOUR; // enable interrupt
}

//SPI initialization
void init_spi(){
	UCB0CTL1 = UCSSEL_2+UCSWRST;  		// Reset state machine; SMCLK source;
	UCB0CTL0 = 					// Data capture on rising edge
			   							// read data while clock high
										// lsb first, 8 bit mode,
			   +UCMST					// master
			   +UCMODE_0				// 3-pin SPI mode
			   +UCSYNC;					// sync mode (needed for SPI or I2C)
	UCB0BR0=BRLO;						// set divisor for bit rate
	UCB0BR1=BRHI;
	UCB0CTL1 &= ~UCSWRST;				// enable UCB0 (must do this before setting
										//              interrupt enable and flags)
	IFG2 &= ~UCB0RXIFG;					// clear UCB0 RX flag
	IE2 |= UCB0RXIE;
	// Connect I/O pins to UCB0 SPI
	P1SEL |=SPI_CLK+SPI_SOMI+SPI_SIMO;
	P1SEL2|=SPI_CLK+SPI_SOMI+SPI_SIMO;
}

//WDT initialization
void init_wdt(){
	// setup the watchdog timer as an interval timer
  	WDTCTL = (WDTPW +		// (bits 15-8) password
     	                   	// bit 7=0 => watchdog timer on
       	                 	// bit 6=0 => NMI on rising edge (not used here)
                        	// bit 5=0 => RST/NMI pin does a reset (not used here)
           	 WDTTMSEL +     // (bit 4) select interval timer mode
  		     WDTCNTCL  		// (bit 3) clear watchdog timer counter
  		                	// bit 2=0 => SMCLK is the source
  		                	// bits 1-0 = 10=> source/512
 			 );
  	IE1 |= WDTIE; // enable WDT interrupt
 }

void interrupt button_handler(){
// check interrupt comes from the desired bit...
// (if not, just ignore -- cannot happen in this case)

		if (P1IFG & BUTTON_BIT_ONE){
			P1IFG &= ~BUTTON_BIT_ONE; // reset the interrupt flag
			digit1 = 1;
			debounce = 32;
			changed = 1;
		}
		else
			P1IFG &= ~BUTTON_BIT_ONE; // reset the interrupt flag
		if (P1IFG & BUTTON_BIT_TWO){
			P1IFG &= ~BUTTON_BIT_TWO; // reset the interrupt flag
			digit2 = 1;
			debounce = 32;
			changed = 1;
		}
		else
			P1IFG &= ~BUTTON_BIT_TWO; // reset the interrupt flag
		if (P1IFG & BUTTON_BIT_THREE){
			P1IFG &= ~BUTTON_BIT_THREE; // reset the interrupt flag
			digit3 = 1;
			debounce = 32;
			changed = 1;
		}
		else
			P1IFG &= ~BUTTON_BIT_THREE; // reset the interrupt flag
		if (P1IFG & BUTTON_BIT_FOUR){
			P1IFG &= ~BUTTON_BIT_FOUR; // reset the interrupt flag
			digit4 = 1;
			debounce = 32;
			changed = 1;
		}
		else
			P1IFG &= ~BUTTON_BIT_FOUR; // reset the interrupt flag
}
ISR_VECTOR(button_handler,".int02") // declare interrupt vector

interrupt void WDT_interval_handler(){
	unsigned char b;
	b = (P1IN & BUTTON);
	if ((started != 1)){
/*		randint = (0xFFFF & rand());		//random integer (function defined below)
		rand2 = (0xFFFF & rand());
		randint = randint + (rand2 << 16);*/
		randint = 0xFFFF;
		data_to_send = 's';
		UCB0TXBUF = data_to_send;
		state = 's';						//starting state
		started = 1;
		tx_mode = 1;						//sets to transmit
		WDT_counter = 1;					//WDT interrupt count
	}

	if (started == 1 && WDT_counter == 0){
		//lower bits state goes into receiving mode and ensures
		//it does not go to the otehr states without knowing
		if ((state == 's') && (rx_mode == 1)){
			data_to_send = 'a';
			rx_mode = 0;					//no longer receiving
			state = 'a';
		}
		//upper bits state goes into receiving mode and ensures
		//it does not go to the otehr states without knowing
		else if (state == 'a'){
			data_to_send = 'b';
			rx_mode = 0;					//no longer receiving
			state = 'b';
		}
		else if (state == 'b'){
			data_to_send = 'c';
			rx_mode = 0;					//no longer receiving
			state = 'c';
		}
		else if (state == 'c'){
			data_to_send = 'd';
			rx_mode = 0;					//no longer receiving
			state = 'd';
		}
		else if (state == 'd'){
			data_to_send = 'e';
			rx_mode = 0;					//no longer receiving
			state = 'e';
		}
		else if (state == 'e'){
			data_to_send = 'f';
			rx_mode = 0;					//no longer receiving
			state = 'f';
		}
		else if (state == 'g'){
			if (read_key == key){
				data_to_send = '=';
				pin = pin1 + pin2;
			}
			else{
				pin = (0xFFFF & rand()) % 10000;
				data_to_send = '!';
			}
			read_key = 0;
			state = 's';					//change back to starting state
			tx_mode = 1;					//goes to transmit mode
		}
		UCB0TXBUF = data_to_send;			//store the data_to_send
		WDT_counter = 1;					//refresh WDT_counter
	}
	if (pin_index == 4 && debounce == 0){
		if (pin == typed_pin){
			P1OUT |= RED;
			lock_timeout = 200;
		}
		pin_index = 0;
		typed_pin = 0;
		pin_digits[0] = 0;
		pin_digits[1] = 0;
		pin_digits[2] = 0;
		pin_digits[3] = 0;
	}
	if (changed == 1 && debounce == 0){
		int testvalue = 0;
		if (digit1 & ~digit2 & digit3 & ~digit4)
			testvalue = 0;
		else{
			if (digit1)
				testvalue += 1;
			if (digit2)
				testvalue += 2;
			if (digit3)
				testvalue += 4;
			if (digit4)
				testvalue += 8;
		}
		if (testvalue <= 9)
			pin_digits[pin_index] = testvalue;
		if (digit1 & digit2 & digit3 & digit4)
			key = read_key;
		else
			pin_index++;
		digit1= digit2= digit3=digit4=0;
		changed = 0;
		typed_pin = pin_digits[3]+pin_digits[2]*10+pin_digits[1]*100+pin_digits[0]*1000;
	}
	if (debounce > 0)
		debounce--;
	if (lock_timeout > 0)
		lock_timeout--;
	else
		P1OUT &= ~RED;
	last_button=b;							//records last button press
	WDT_counter--;

}
ISR_VECTOR(WDT_interval_handler, ".int10")

//spi interrupt handler for the master, handles the inputs
void interrupt spi_rx_handler(){
	//if transmitting, turn it off and go into receiving mode
	if (tx_mode){
		tx_mode = 0;
		rx_mode = 1;
	}
	//retrieves lower bits from receive buffer
	else if (state == 'a'){
		data_received = UCB0RXBUF;
		first_byte = data_received;
	}
	//retrieves upper bits from the receive buffer
	else if (state == 'b'){
		data_received = UCB0RXBUF;
		second_byte = data_received << 8;		//shifts the upper bits into position
	}
	else if (state == 'c'){
		data_received = UCB0RXBUF;
		third_byte = data_received << 16;		//shifts the upper bits into position
	}
	else if (state == 'd'){
		data_received = UCB0RXBUF;
		fourth_byte = data_received << 24;		//shifts the upper bits into position
		read_key = fourth_byte + third_byte + second_byte + first_byte;	//creates the complete key
	}
	else if (state == 'e'){
		data_received = UCB0RXBUF;
		pin1 = data_received;		//shifts the upper bits into position
	}
	else if (state == 'f'){
		data_received = UCB0RXBUF;
		pin2 = data_received << 8;		//shifts the upper bits into position
		state = 'g';							//changes to key confirm state
	}
	IFG2 &= ~UCB0RXIFG;							//turns off flag
}
ISR_VECTOR(spi_rx_handler, ".int07")

/**
* Random number generator.
*
* NOTE: This affects Timer A.
*
* Algorithm from TI SLAA338:
* http://www.ti.com/sc/docs/psheets/abstract/apps/slaa338.htm
*
* @return 16 random bits generated from a hardware source.
*/
#ifndef __RAND_H
#define __RAND_H
//LCG Constants
#define M 49381 // Multiplier
#define I 8643 // Increment
#endif

unsigned int rand(void) {
	int i, j;
	unsigned int result = 0;
	//saves the state
	unsigned int BCSCTL3_old = BCSCTL3;
	unsigned int TACCTL0_old = TACCTL0;
	unsigned int TACTL_old = TACTL;

	//Stops the timer
	TACTL = 0x0;
	//sets up the timer
	BCSCTL3 = (~LFXT1S_3 & BCSCTL3) | LFXT1S_2; // Source ACLK from VLO
	TACCTL0 = CAP | CM_1 | CCIS_1; // Capture mode, positive edge
	TACTL = TASSEL_2 | MC_2; // SMCLK, continuous up

	//Generates bits
	for (i = 0; i < 16; i++) {
		unsigned int ones = 0;
		for (j = 0; j < 5; j++) {
			while (!(CCIFG & TACCTL0)); // Wait for interrupt
				TACCTL0 &= ~CCIFG; // Clear interrupt
			if (1 & TACCR0) // If LSb set, count it
				ones++;
		}
		result >>= 1; // Save previous bits
		if (ones >= 3) // Best out of 5
			result |= 0x8000; // Set MSb
	}
	//Restores to the original state
	BCSCTL3 = BCSCTL3_old;
	TACCTL0 = TACCTL0_old;
	TACTL = TACTL_old;
	return result;
}
