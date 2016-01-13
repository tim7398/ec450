/***************************************************************************************
 *  Simple Button Example (version 1)
 *
 * In operation either the RED or GREEN LED is on.  Pressing the Launchpad button toggles
 * between them.  A global counter keeps track of the total number of button presses.
 *
 *  This version uses the Watchdog Timer ("WDT") in interval mode.
 *  The default system clock is about 1.1MHz.  The WDT is configured to be driven by this
 *  clock and produce an interrupt every 8K ticks ==> interrupt interval = 8K/1.1Mhz ~ 7.5ms
 *  When the WDT interrupt occurs, the WDT interrupt handler is called.
 *  This handler checks the state of the button and if it has been *pressed* since the last
 *  interrupt it toggles the two LEDs.
 *
 * Initially the green LED is on and the red LED is off.
 *
 *  NOTE: Between interrupts the CPU is OFF!
 ***************************************************************************************/

#include <msp430g2553.h>


// Bit masks for port 1
#define RED 0x01
#define GREEN 0x40
#define BUTTON 0x08
volatile unsigned int Recorder = 0;
volatile unsigned int counter = 0; // keeps track of time of button untouched
volatile unsigned int playback = 0; //flag to determine mode
volatile unsigned int array[50]; // stores the delays
volatile unsigned int crash_counter = 0;
volatile unsigned int tracker = 0;
volatile unsigned int pattern_counter = 0;
volatile unsigned int playback_counter=2;
volatile unsigned int run_time = 0;
// Global state variables
volatile unsigned char last_button; // the state of the button bit at the last interrupt

void main(void) {
	  // setup the watchdog timer as an interval timer
	  WDTCTL =(WDTPW + // (bits 15-8) password
	                   // bit 7=0 => watchdog timer on
	                   // bit 6=0 => NMI on rising edge (not used here)
	                   // bit 5=0 => RST/NMI pin does a reset (not used here)
	           WDTTMSEL + // (bit 4) select interval timer mode
	           WDTCNTCL +  // (bit 3) clear watchdog timer counter
	  		          0 // bit 2=0 => SMCLK is the source
	  		          +1 // bits 1-0 = 01 => source/8K
	  		   );
	  IE1 |= WDTIE;		// enable the WDT interrupt (in the system interrupt register IE1)

	  P1DIR |= RED;
	  P1DIR |= GREEN;					// Set RED and GREEN to output direction
	  P1DIR &= ~BUTTON;						// make sure BUTTON bit is an input
	  P1OUT &= ~RED;
	  P1OUT &= ~GREEN;	// sets led off initially
	  P1OUT |= BUTTON;
	  P1REN |= BUTTON;						// Activate pullup resistors on Button Pin

	  _bis_SR_register(GIE+LPM0_bits);  // enable interrupts and also turn the CPU off!
}

// ===== Watchdog Timer Interrupt Handler =====
// This event handler is called to handle the watchdog timer interrupt,
//    which is occurring regularly at intervals of about 8K/1.1MHz ~= 7.4ms.

interrupt void WDT_interval_handler(){
  	unsigned char b;


  	if (playback == 0 )
  	{
  		b = (P1IN & BUTTON);  // read the BUTTON bit
  		if ((b == 0) && (Recorder<49))	// button is pressed
  		{
  			P1OUT |= RED; // turn on led
  			tracker++;
  		}
  		else if ((b!=0) && (Recorder < 49))
  		  		{

  		  			P1OUT &= ~RED;	// if button isnt pressed off
  		  			counter++;
  		  		}

  		if ((last_button!=0) && (b==0) && (Recorder < 49)) // pressed button first
  		{
  			array[Recorder]=counter; // store length of no led
  			Recorder++;
  			counter = 0;	//clear the counter for no led


  		}

  		if ((last_button==0) && (b!=0)&& (Recorder < 49)) // releasing the button, low to high
  		{
  			array[Recorder]=tracker;// store length of led on
  			tracker = 0; // clear timer for led on
  			Recorder++;
  		}

  		if(Recorder >= 49 ) // if reach end of array
  		{
  			P1OUT &= ~RED;
  			P1OUT |= GREEN;
  			crash_counter++;
  			counter=0;
  		}

  		if (crash_counter == 200) // green led on for 2 seconds
  		{
  			crash_counter = 0;	// reset all the counters
  			P1OUT &= ~GREEN; 	// turn off led
  			Recorder = 0;
  			counter = 0;
  			tracker = 0;
  			playback=0; // MIGHT CHANGE THIS BACK TO 1
  		}



  		last_button=b;    // remember button reading for next time.


  		if(counter == 1000)
  		{
  			run_time++;
  			if (run_time == 1)
  			{
  				pattern_counter=array[1];
  			}
  			else
  			{
  				pattern_counter=array[0];
  			}
  			P1OUT |= GREEN;
  			counter=0;
  			tracker=0;
			playback = 1;

  		}
  	}




  	else
  	{


  		if(run_time == 1)
  		{
  			if (pattern_counter==0)
  				{          // decrement the counter and act only if it has reached 0
  					P1OUT ^= 1;
  					pattern_counter=array[playback_counter]; // reset the down counter
  					playback_counter++;


  				}
  	                    // toggle LED on P1.0
    		if (playback_counter==(Recorder+1))
    			{
    				playback = 0;
    				Recorder = 0;
    				playback_counter=1;
    				P1OUT &= ~GREEN;
    			}
    		pattern_counter--;
  	}

  		else
  		{

  			if (pattern_counter==0)
  		  				{          // decrement the counter and act only if it has reached 0
  		  					P1OUT ^= 1;
  		  					pattern_counter=array[playback_counter]; // reset the down counter
  		  					playback_counter++;


  		  				}
  		  	                    // toggle LED on P1.0
  		    		if (playback_counter==(Recorder+1))
  		    			{
  		    				playback = 0;
  		    				Recorder = 0;
  		    				playback_counter=1;
  		    				P1OUT &= ~GREEN;
  		    			}
  		    		pattern_counter--;
  		}
  	}



}
// DECLARE function WDT_interval_handler as handler for interrupt 10
// using a macro defined in the msp430g2553.h include file
ISR_VECTOR(WDT_interval_handler, ".int10")
