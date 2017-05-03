/*
 * LightsController_2017.c
 *
 * Created: 18/03/17 15:05:46
 * Author : Sondre
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "UniversalModuleDrivers/can.h"
#include "UniversalModuleDrivers/rgbled.h"
#include "UniversalModuleDrivers/timer.h"
#include "BlinkLights.h"

int main(void)
{
	can_init(0, 0);
	rgbled_init();
	timer_init();
	sei();
	
	timer_start(TIMER3);
	
	DDRE |= 1<<PE3; //Initiate (output) PE3, Front Right
	DDRE |= 1<<PE4; //Initiate (output) PE4, Front Left
	DDRE |= 1<<PE5; //Initiate (output) PE5, Front Right blinker
	DDRB |= 1<<PB4; //Initiate (output) PB4, Front Left blinker
	
	PORTE &= ~(1 << PE3);
	PORTE &= ~(1 << PE4);
	PORTE &= ~(1 << PE5);
	PORTB &= ~(1 << PB4);
	
	int count_right;
	int count_left;
	
    while (1) 
    {	
		CanMessage_t rxFrame;
		if (can_read_message_if_new(&rxFrame)) {
			rgbled_toggle(LED_ALL);
			
			if (rxFrame.id == 0x310) {
				if (rxFrame.data[0]) {
					PORTE |= (1 << PE3); //Turn off or on head lights
					PORTE |= (1 << PE4);
				} else {
					PORTE &= ~(1 << PE3);
					PORTE &= ~(1 << PE4);
				}
			} else if (rxFrame.id == 0x230) {
				if (rxFrame.data[1] & (1 << 4)) {
					count_left = 0;
					count_right = 1;
				} 
				if (rxFrame.data[1] & (1 << 3)) {
					count_right = 0;
					count_left = 1;
				}
				
				if (((timer_elapsed_ms(TIMER3) >= 500) && (rxFrame.data[1] & (1 << 4))) || ((timer_elapsed_ms(TIMER3)>= 500) && count_right > 0)) {
					if (count_right > 0 && count_right <= 6) {
						PORTB &= ~(1 << PB4);
						count_left = 0;
						PORTE ^= (1 << PE5);
						count_right++;
					} else if (count_right > 6) {
						PORTB &= ~(1 << PB4);
						count_left = 0;
						PORTE &= ~(1 << PE5);
						count_right = 0;
					} else {
						PORTB &= ~(1 << PB4);
						count_left = 0;
						PORTE ^= (1<< PE5);
					}
				}
				
				if (((timer_elapsed_ms(TIMER3) >= 500) && (rxFrame.data[1] & (1 << 3))) || ((timer_elapsed_ms(TIMER3)>= 500) && count_left > 0)) {
					if (count_left > 0 && count_left <= 6) {
						PORTE &= ~(1 << PE5);
						count_right = 0;
						PORTB ^= (1 << PB4);
						count_left++;
					} else if (count_left > 6) {
						PORTB &= ~(1 << PB4);
						count_left = 0;
						PORTB &= ~(1 << PB4);
						count_left = 0;
					} else {
						PORTE &= ~(1 << PE5);
						count_right = 0;
						PORTB ^= (1<< PB4);
					}
				}
				
				if (timer_elapsed_ms(TIMER3) >= 500) {
					timer_start(TIMER3);
				}
			}
		}
		_delay_ms(1);
    }
}

