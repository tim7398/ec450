#include "msp430g2553.h"

//port declarations
#define RED 0x01
#define SPI_CLK 0x20
#define SPI_SOMI 0x40
#define SPI_SIMO 0x80
#define BRLO (BIT_RATE_DIVISOR &  0xFF)
#define BRHI (BIT_RATE_DIVISOR / 0x100)
#define ACTION_INTERVAL 1
#define BIT_RATE_DIVISOR 32

//global variables
volatile unsigned int data_received = 0;
volatile unsigned int guess = 32767;
volatile unsigned int upper = 65535;
volatile unsigned int lower = 0;
volatile unsigned int num_guess = 0;

//function declarations
void init_spi(void);
void init_wdt(void);

//main program
void main() {

	WDTCTL = WDTPW + WDTHOLD;       // Stop watchdog timer
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL = CALDCO_1MHZ;

	P1DIR |= RED;					//LED initialization
	P1OUT &= ~RED;

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

//SPI interrupt handler
void interrupt spi_rx_handler(){
	data_received = UCB0RXBUF;						//takes in data from master

	//data received states
	//s = start, l = lower bits, u = upper bits
	if (data_received == 's'){						//sends the lower bits
		guess = 0x0FFF;
		UCB0TXBUF = guess & 0x00FF;
		lower = 0;
		upper = 65535;
	}
	else if (data_received == 'l'){					//sends the upper bits
		UCB0TXBUF = guess >> 8;
	}
	else if (data_received == 'u'){					//intermediary signal
		UCB0TXBUF = 0;								//sends dummy input
	}
	else{											//guessing state, uses binary search
		if (data_received == '<'){					// if less than
			lower = guess+1;						//raise window above guess
			guess = (upper - lower)/2 + lower;		//recalculate guess
		}
		else if (data_received == '>'){				//if greater than
			upper = guess-1;						//lower window below guess
			guess = (upper - lower)/2 + lower;		//recalc
		}
		else if (data_received == '='){				//equal to
			P1OUT &= ~RED;							//turn on LED (will be toggled on after)
		}
		UCB0TXBUF = guess & 0x00FF;					//sends lower bits
		num_guess++;								//increases number of guesses
	}
	P1OUT ^= RED;									//toggles LED (to tell if guessing)
	IFG2 &= ~UCB0RXIFG;								//turn of flag
}
ISR_VECTOR(spi_rx_handler, ".int07")

