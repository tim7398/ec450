	asm(" .length 10000");
	asm(" .width 132");
	// Oct 2015
	// Tone4 == produces a variable 1 Khz tone using TimerA, Channel 0
	// Toggled on and off with a button
	//-----------------------
	#include "msp430g2553.h"
	//-----------------------
	// define the bit mask (within P1) corresponding to output TA0
	#define TA0_BIT 0x02 //pin 1.1
	#define Restart 0x04 //pin 1.2
	// define the port and location for the button (this is the built in button)
	// specific bit for the button
	#define BUTTON_BIT 0x08 // pin 1.3
	#define Select 0x10 //pin 1.4
	#define Speed 0xA0
	//----------------------------------


	// Some global variables (mainly to look at in the debugger)

	const int notes[]={955, 1012, 1136, 1275, 1431, 1516, 1702, 1910, 1275, 1136, 0, 1136, 1012, 0, 1012, 955, 0, 955, 0, 955, 1012, 1136, 1275, 0, 1275, 1431, 1516, 955, 0, 955, 1012, 1136, 1275, 0, 1275, 1431, 1516, 0, 1516, 0, 1516, 0, 1516, 0, 1516, 0, 1516, 1431, 1275, 1431, 1516, 1702, 0, 1702, 0, 1702, 0, 1702, 1516, 1431, 1516, 1702, 1516, 955, 1136, 1275, 1431, 1516, 1431, 1516, 1702, 1910, 0};
	const int notes2[]={1910,955,1012,955,955,1431,1012,0,1073,1012,1012,1431,1073,1136,1073,1136,1073, 955, 1275,1136,0,1136,0,1136,0,1136,1431,1275,0,1275,1431,1200,1431,1136,0,1136,1073,1136,1275,1136,851,1136,1431,1702,955,955,851,905,851,758,716,758,851,955,955,1073,1136,1073,1136,1073,955,1136,1204,1136,1204,1136,1073,1275,1351,1275,1351,1275,1136,1431,1136,851,851,955,905,851,758,716};
	const int notelength[]= {1000, 708,  210,  935,  334,  630,  562,  713,  374, 1050, 1,  420, 1180, 1,  472, 625, 1, 250, 1, 250,  472,  420,  374, 1,  561,  167,  315, 500, 1, 500,  472,  420,  374, 1,  561,  167,  315, 1,  315, 1,  315, 1,  315, 1,  315, 1,  315,  167,  935,  167,  158,  281, 1,  281, 1,  281, 1,  141,  158,  835,  158,  141,  315, 500,  420,  561,  167,  315,  334,  630,  562, 1000, 10};
	const int notelength2[]={500, 500, 250,250,500,250,500,1, 250,250,500,250,250,250,250,250,250,350,350,250,1,250,1,250,1,700,125,250,1,250,350,350,350,250,1,350,400,600,400,330,330,250,250,250,250,1000,400,250,400,250,400,250,400,250,1000,400,250,400,250,250,600,400,250,400,250,250,600,400,250,400,250,250,600,400,250,250,500,600,600,600,600,2000};
	volatile unsigned notecounter = 0, length = 0, state =0,button_counter=0,button_counter2=0,button_counter3=0,pauselength=0,pausecounter=0,debouncer=0,scale_state=0;
	volatile double scale = 1;

	void init_timer(void);  // routine to setup the timer
	void init_button(void); // routine to setup the button

	// ++++++++++++++++++++++++++
	void main(){
		WDTCTL = WDTPW + WDTHOLD; // Stop watchdog timer
		BCSCTL1 = CALBC1_1MHZ; // 1Mhz calibration for clock
		DCOCTL = CALDCO_1MHZ;

		halfPeriod=maxHP; // initial half-period at lowest frequency
		init_timer(); // initialize timer
		init_button(); // initialize the button
		_bis_SR_register(GIE+LPM0_bits);// enable general interrupts and power down CPU
	}

	// +++++++++++++++++++++++++++
	// Sound Production System
	void init_timer(){ // initialization and start of timer
		TA0CTL |=TACLR; // reset clock
		TA0CTL =TASSEL1+ID_0+MC_2; // clock source = SMCLK, clock divider=1, continuous mode,
		TA0CCTL0=CCIE; // compare mode, outmod=0, interrupt CCR0 on
		TA0CCR0 = TAR+notes[0]; // time for first alarm
		TA0CCR0 = TAR+notes2[0];
		P1SEL|=TA0_BIT; // connect timer output to pin
		P1DIR|=TA0_BIT;
	}

	// +++++++++++++++++++++++++++
	void interrupt sound_handler(){
		if(state == 0)
		{
		TACCR0 += notes[notecounter]; // advance 'alarm' time

			length++; 			// adjust the period
			if (length>=(notelength[notecounter]*scale))
			{			// check min
				length=0;			// reverse direction
				notecounter++;			// stay at minHP
			}
			if(notecounter == (sizeof(notes)/sizeof(int)))
			{
				notecounter = 0;


			}

		}
		else if (state == 1)
		{
			TACCR0 += notes2[notecounter]; // advance 'alarm' time
			//if (SoundOn){ // change half period if the outmod is not 0
				length++; 			// adjust the period
				if(scale_state==1)
				{
					if ((length*2)>=(notelength2[notecounter]))
					{			// check min
						length=0;			// reverse direction
						notecounter++;			// stay at minHP
					}
					if(notecounter == (sizeof(notes2)/sizeof(int)))
					{
						notecounter = 0;
					}
				}
				if(scale_state==2)
							{
								if ((length)>=(notelength2[notecounter]*2))
								{			// check min
									length=0;			// reverse direction
									notecounter++;			// stay at minHP
								}
								if(notecounter == (sizeof(notes2)/sizeof(int)))
								{
									notecounter = 0;
								}
							}
				if(scale_state==0)
								{
									if ((length)>=(notelength2[notecounter]))
									{			// check min
										length=0;			// reverse direction
										notecounter++;			// stay at minHP
									}
									if(notecounter == (sizeof(notes2)/sizeof(int)))
									{
										notecounter = 0;
									}
								}
			//}
		}
		++intcount; // advance interrupt counter for debugging
	}
	ISR_VECTOR(sound_handler,".int09") // declare interrupt vector

	// +++++++++++++++++++++++++++
	// Button input System
	// Button toggles the state of the sound (on or off)
	// action will be interrupt driven on a downgoing signal on the pin
	// no debouncing (to see how this goes)

	void init_button(){
	// All GPIO's are already inputs if we are coming in after a reset
		if(BUTTON_BIT) // start and pause button intialization
		{
		P1OUT |= BUTTON_BIT; // pullup
		P1REN |= BUTTON_BIT; // enable resistor
		P1IES |= BUTTON_BIT; // set for 1->0 transition
		P1IFG &= ~BUTTON_BIT;// clear interrupt flag
		P1IE  |= BUTTON_BIT; // enable interrupt
		}
		if(Restart)//restart button initialize
		{
			P1OUT |= Restart; // pullup
			P1REN |= Restart; // enable resistor
			P1IES |= Restart; // set for 1->0 transition
			P1IFG &= ~Restart;// clear interrupt flag
			P1IE  |= Restart; // enable interrupt
		}
		if(Select)
		{
			P1OUT |= Select; // pullup
			P1REN |=  Select; // enable resistor
			P1IES |=  Select; // set for 1->0 transition
			P1IFG &= ~Select;// clear interrupt flag
			P1IE  |= Select; // enable interrupt
		}
		if(Speed) // start and pause button intialization
			{
			P1OUT |= Speed; // pullup
			P1REN |=  Speed; // enable resistor
			P1IES |=  Speed; // set for 1->0 transition
			P1IFG &= ~Speed;// clear interrupt flag
			P1IE  |=  Speed; // enable interrupt
			}
	}
	void interrupt button_handler(){
	// check that this is the correct interrupt
	// (if not, it is an error, but there is no error handler)
		debouncer++;

		if(debouncer >= 5000)
		{
			debouncer = 0;
		if (P1IFG & BUTTON_BIT)
		{

				if(button_counter==0)// press for sound. loads last saved location
					{
					P1IFG &= ~BUTTON_BIT; // reset the interrupt flag
					TACCTL0 ^= OUTMOD_4; // flip bit for outmod of sound
					length=pauselength;
					notecounter=pausecounter;
					button_counter++;

					}
				else // if button pressed, save the current location
					{
					P1IFG &= ~BUTTON_BIT; // reset the interrupt flag
					TACCTL0 ^= OUTMOD_4; // flip bit for outmod of sound
					pauselength=length+1;
					pausecounter=notecounter+1;
					button_counter=0;

					}
			}




		if(P1IFG & Restart) // resets all counters to 0, this replays song from beginning
		{
			P1IFG &= ~Restart; // reset the interrupt flag
			length = 0;
			notecounter=0;
			scale = 1;
			scale_state = 0;
		}

		if(P1IFG & Select) // changes state, which changes which songs playing. plays each song from the beginning
		{
			P1IFG &= ~Select;
			if(button_counter2==0)
			{
				state = 1;
				length = 0;
				notecounter=0;
				button_counter2++;

			}
			else
			{
				state = 0;
				length = 0;
				notecounter=0;
				button_counter2 = 0;

			}
		}
		if(P1IFG & Speed)// speeds up or slows down the song
				{
					P1IFG &= ~Speed; // reset the interrupt flag
					if(button_counter3==0)
					{
						scale = 0.5;
						button_counter3++;
						scale_state = 1;
					}

					else
					{
						scale_state = 2;
						scale = 2;
						button_counter3=0;
					}

				}
	}
	}

	ISR_VECTOR(button_handler,".int02") // declare interrupt vector
	// +++++++++++++++++++++++++++
