#include "msp430g2553.h"

//port declarations
#define RED 0x01
#define LED1 0x02
#define LED2 0x04
#define LED3 0x08
#define LED4 0x10
#define SPI_CLK 0x20
#define SPI_SOMI 0x40
#define SPI_SIMO 0x80
#define BRLO (BIT_RATE_DIVISOR &  0xFF)
#define BRHI (BIT_RATE_DIVISOR / 0x100)
#define ACTION_INTERVAL 1
#define BIT_RATE_DIVISOR 32

//global variables
volatile unsigned int data_received = 0;
volatile unsigned long key = 0xFFFF;
volatile unsigned int WDT_counter = 192;
volatile unsigned char pindigit = 0;
volatile unsigned char playpin = 0;
volatile unsigned int pin = 0;

//function declarations
void init_spi(void);
void init_wdt(void);
unsigned int rand(void);

//main program
void main() {

	WDTCTL = WDTPW + WDTHOLD;       // Stop watchdog timer
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL = CALDCO_1MHZ;

	P1DIR |= RED;					//LED initialization
	P1OUT &= ~RED;
	P1DIR |= LED1;					//LED initialization
	P1OUT &= ~LED1;
	P1DIR |= LED2;					//LED initialization
	P1OUT &= ~LED2;
	P1DIR |= LED3;					//LED initialization
	P1OUT &= ~LED3;
	P1DIR |= LED4;					//LED initialization
	P1OUT &= ~LED4;

	pin =  1234;//(0xFFFF & rand()) % 10000;		//random integer (function defined below)
	playpin = 0;

	init_wdt();
	init_spi();
	_bis_SR_register(GIE + LPM0_bits);
}

//SPI initialization
void init_spi(){
	UCB0CTL1 = UCSWRST;  		// Reset state machine; SMCLK source;
	UCB0CTL0 = 					// Data capture on rising edge
			   							// read data while clock high
										// lsb first, 8 bit mode,
										// slave
			   +UCMODE_0				// 3-pin SPI mode
			   +UCSYNC;					// sync mode (needed for SPI or I2C)
	UCB0BR0=BRLO;						// set divisor for bit rate
	UCB0BR1=BRHI;
	UCB0CTL1 &= ~UCSWRST;				// enable UCB0 (must do this before setting
										//              interrupt enable and flags)
	IFG2 &= ~UCB0RXIFG;					// clear UCB0 RX flag
	IE2 |= UCB0RXIE;					// enable UCB0 RX interrupt
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

//SPI interrupt handler
void interrupt spi_rx_handler(){
	data_received = UCB0RXBUF;						//takes in data from master

	//data received states
	//s = start, l = lower bits, u = upper bits
	if (data_received == 's'){						//sends the first byte
		key = 0xFFFF;
		UCB0TXBUF = key & 0x000000FF;
	}
	else if (data_received == 'a'){					//sends the second byte
		UCB0TXBUF = key >> 8;
	}
	else if (data_received == 'b'){					//sends the third byte
		UCB0TXBUF = key >> 16;
	}
	else if (data_received == 'c'){					//sends the fourth byte
		UCB0TXBUF = key >> 24;
	}
	else if (data_received == 'd'){					//sends the lower pin
		UCB0TXBUF = pin & 0x00FF;
	}
	else if (data_received == 'e'){					//sends the upper pin
		UCB0TXBUF = pin >> 8;
	}
	else if (data_received == 'f'){					//intermediary signal
		UCB0TXBUF = 0;								//sends dummy input
	}
	else{
		//RIGHT HERE, SEND PIN CODE FOR RESETS
		if (data_received == '='){				//equal to
			playpin = 1;
			P1OUT &= ~RED;							//turn on LED (will be toggled on after)
		}
		else{
			playpin = 0;
			pindigit = 0;
		}
		UCB0TXBUF = key & 0x000000FF;					//sends lower bits
	}
	//P1OUT ^= RED;									//toggles LED (to tell if keying)
	IFG2 &= ~UCB0RXIFG;								//turn of flag
}
ISR_VECTOR(spi_rx_handler, ".int07")

interrupt void WDT_interval_handler(){
	if ((playpin) == 1 && (WDT_counter %32 == 0) && pindigit < 4){
		//P1OUT |= LED1 + LED2 + LED3 + LED4;
		switch (pindigit){
			case 0: P1OUT |= LED4;
					P1OUT &= ~(LED1 + LED2 + LED3);

				break;
			case 1: P1OUT |= LED3;
					P1OUT &= ~(LED1 + LED2 + LED4);

				break;
			case 2: P1OUT |= LED3 + LED4;
					P1OUT &= ~(LED1 + LED2);

				break;
			case 3: P1OUT |= LED2;
					P1OUT &= ~(LED1 + LED3 + LED4);

				break;
		}
		pindigit++;
	}
	else if (WDT_counter % 16 == 0){
		P1OUT &= ~(LED1 + LED2 + LED3 + LED4);
	}
	if (WDT_counter == 0){
		WDT_counter = 192;
		pindigit = 0;
	}

	WDT_counter--;
}
ISR_VECTOR(WDT_interval_handler, ".int10")

//For in case you want to set a random pin to send to the master instead.
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
