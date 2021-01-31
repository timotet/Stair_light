/*
 * 1/3/2021
 * PIR sensor switch for LED lighting
 * in my steep dark stair well
 *
 * Sensor triggers interrupt, turns on an led and sets timerA
 * to run for 30 seconds roughly.
 * then turns off led and goes into low power mode and waits.
 *
 */

#include <msp430.h> 
#include <stdbool.h>

#define PIR  BIT7          //PIR sensor on P1.7

#define LED      BIT4                         // transistor
#define LED_on   P1OUT |= LED
#define LED_off  P1OUT &= ~LED

#define rLed          BIT0                // on board leds for debug
#define rLed_on       P1OUT |= rLed
#define rLed_off      P1OUT &= ~rLed
#define rLed_toggle   P1OUT ^= rLed

#define gLed          BIT6
#define gLed_on       P1OUT |= gLed
#define gLed_off      P1OUT &= ~gLed
#define gLed_toggle   P1OUT ^= gLed

#define notUsedGPIOP1   BIT1 | BIT2 | BIT3 | BIT5   // Turn off all unused gpio's port 1
#define notUsedGPIOP2   BIT6 | BIT7                 // Turn off all unused gpio's port 2

volatile bool PIR_flag = false;
volatile bool timer_flag = true;
volatile unsigned int seconds = 0;
volatile unsigned int ticks = 0;


///functions
static void P1_init(void){

    P1DIR |= rLed | gLed | LED | notUsedGPIOP1;      //set as output
    P2SEL &= ~notUsedGPIOP2;         //initialize port2
    P2DIR |= notUsedGPIOP2;          //set as output
    P1DIR &= ~PIR;                   //set PIR as input
    P1OUT &= ~(rLed | gLed | LED);   //set leds low
    P1IN  |= PIR;                    //input signal == HIGH
    P1OUT &= ~PIR;                   //pull down for PIR
    P1REN &= ~notUsedGPIOP1;         //pull down unused GPIO's
    P2REN &= ~notUsedGPIOP2;
    P1REN |= PIR;                    //internal resistor enable
    P1IES &= ~PIR;           //Interrupt Edge Select - 0: trigger on rising edge, 1: trigger on falling edge
    P1IFG &= PIR;            //clear interrupt flag for port 1
    P1IE |= PIR;             //enable port 1 interrupt
}

static void LEDS_on(void) {

	TACTL |= TASSEL_2 + MC_1 + ID_3; // up mode count to CCR0 divide by 8
	CCR0 = 61500;
	rLed_on;
	LED_on;
}

static void LEDS_off(void) {

	CCR0 = 0;                           //timer off
	TACTL |= MC_0 + TACLR;
	LED_off;                            // Leds off
	rLed_off;
	gLed_off;
	LPM4;
}

///Interrupts
#pragma vector = TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void) {

//	TACTL &= ~TAIFG;             //clear timer interrupt
	gLed_toggle;
	ticks++;
	if (ticks == 2){
		ticks = 0;
		seconds ++;
	}
	if (seconds >= 30) {
		seconds = 0;
		ticks = 0;
		timer_flag = false;
	}
}

#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void) {

	LPM4_EXIT;                //exit low power mode 4
	P1IFG &= ~BIT7;           // clear interrupt flag
	PIR_flag = true;
}

///main.c
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	BCSCTL1 = CALBC1_1MHZ;     //calibrate system clock to 1mhz
	DCOCTL = CALDCO_1MHZ;

	CCTL0 = CCIE;              // CCR0 interrupt enabled

	P1_init();                 // set up port 1

	_BIS_SR(LPM4_bits + GIE);  //go into low power mode wait for port 1 interrupt

	while (1) {

		if (PIR_flag) {

			LEDS_on();
			PIR_flag = false;
		}

		else if (!timer_flag) {

			LEDS_off();
			timer_flag = true;
		}
	}
}
