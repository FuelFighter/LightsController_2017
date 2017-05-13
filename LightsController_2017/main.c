/*
 * LightsController_2017.c
 *
 * Created: 11/04/17 13:53:21
 *  Author: Sondre
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "UniversalModuleDrivers/can.h"
#include "UniversalModuleDrivers/rgbled.h"
#include "UniversalModuleDrivers/timer.h"
#include "UniversalModuleDrivers/pwm.h"

CanMessage_t rxFrame;
CanMessage_t txFrame;

uint8_t lightlvl = 0;

int main(void)
{
	can_init(0, 0);
	rgbled_init();
	timer_init();
	pwm_init();
	pwm_set_prescale(SCALE_256,PWM_T2);
	pwm_set_prescale(SCALE_256,PWM_T3);
	pwm_set_top_t3(0xFF);
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
	
	while (1) {
		if (can_read_message_if_new(&rxFrame)) {
			rgbled_toggle(LED_ALL);
			
			if (rxFrame.id == DASHBOARD_CAN_ID) {
				if (rxFrame.data.u8[0] & (1<<0)) {
					pwm_set_duty_cycle(PWM_PE3,lightlvl);
					pwm_set_duty_cycle(PWM_PE4,lightlvl);
							
					} else {
					pwm_set_duty_cycle(PWM_PE3,lightlvl);
					pwm_set_duty_cycle(PWM_PE4,lightlvl);
					}
				lightlvl = rxFrame.data.u8[1];
				
			} else if (rxFrame.id == STEERING_WHEEL_CAN_ID) {
				if (rxFrame.data.u8[1] & (1 << 4)) {
					count_left = 0;
					count_right = 1;
				}
				if (rxFrame.data.u8[1] & (1 << 3)) {
					count_right = 0;
					count_left = 1;
				}
				
				if (((timer_elapsed_ms(TIMER3) >= 500) && (rxFrame.data.u8[1] & (1 << 4))) || ((timer_elapsed_ms(TIMER3)>= 500) && count_right > 0)) {
					if (count_right > 0 && count_right <= 6) {
						pwm_set_duty_cycle(PWM_PB4,0xFF);
						count_left = 0;
						pwm_set_duty_cycle(PWM_PE5,0);
						count_right++;
					} 
					else if (count_right > 6) {
						pwm_set_duty_cycle(PWM_PB4,0xFF);
						count_left = 0;
						pwm_set_duty_cycle(PWM_PB4,0xFF);
						count_right = 0;
					} 
					else {
						pwm_set_duty_cycle(PWM_PB4,0xFF);
						count_left = 0;
						pwm_set_duty_cycle(PWM_PE5,0);
					}
				}
				
				if (((timer_elapsed_ms(TIMER3) >= 500) && (rxFrame.data.u8[1] & (1 << 3))) || ((timer_elapsed_ms(TIMER3)>= 500) && count_left > 0)) {
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

