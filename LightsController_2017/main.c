/*
 * TestProject.c
 *
 * Created: 12.04.2017 11:22:52
 * Author : Ultrawack
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "UniversalModuleDrivers/usbdb.h"
#include "UniversalModuleDrivers/rgbled.h" 
#include "UniversalModuleDrivers/pwm.h"
#include "UniversalModuleDrivers/can.h"
#include "UniversalModuleDrivers/timer.h"

#define BR PWM_PE3
#define FR PWM_PE4
#define BL PWM_PE5
#define FL PWM_PB4	

typedef enum {
	FRONT,
	REAR
} LightModule_t;

typedef enum {
	OFF = 0,
	NORMAL,
	BRAKE
} LightMode_t;

typedef enum {
	NONE = 0,
	LEFT,
	RIGHT,
	HAZARD	
} LightBlinkMode_t;

#define BLINK_COUNT 4
#define LIGHTS_ON_BIT		0b00000001
#define HAZARD_BIT			0b00000100
#define LEFT_BLINKER_BIT	0b00001000
#define RIGHT_BLINKER_BIT	0b00010000


//DEFINE THE PARAMETERS HERE
uint8_t blink_period = 250;	
LightBlinkMode_t BLINK_STATE = NONE;
LightMode_t LIGHT_STATE = OFF;
LightModule_t LIGHT_MODULE = REAR;
uint8_t light_level = 0;
uint8_t blink_decrement = BLINK_COUNT;
CanMessage_t rxFrame;

void handle_can();
void blink_left();
void blink_right();
void hazards();

int main(void)
{
	can_init(0,0);
	rgbled_init();
	timer_init();
	pwm_init();
	pwm_set_prescale(SCALE_256,PWM_T2);
	pwm_set_prescale(SCALE_256,PWM_T3);
	pwm_set_top_t3(0xFF);
	sei();
	
	while(1)
	{
		handle_can();
		switch(LIGHT_STATE)
		{
			case OFF:
			rgbled_turn_on(LED_RED);
			pwm_set_duty_cycle(BR,0x00);
			pwm_set_duty_cycle(BL,0x00);
			pwm_set_duty_cycle(FR,0x00);
			pwm_set_duty_cycle(FL,0x00);
			break;
			
			case BRAKE:
			if (LIGHT_MODULE == REAR)
			{
				pwm_set_duty_cycle(FL,0xFF);
				pwm_set_duty_cycle(FR,0x0F);
			}
			case NORMAL:
			if ((LIGHT_STATE != BRAKE) && (LIGHT_MODULE == REAR)){
				pwm_set_duty_cycle(FL,0x0F);
				pwm_set_duty_cycle(FR,0xF0);
			} else if (LIGHT_MODULE == FRONT){
				pwm_set_duty_cycle(FL,light_level);
				pwm_set_duty_cycle(FR,light_level);
			}
			
			switch (BLINK_STATE)
			{
				case LEFT:
				blink_left();
				break;	
				case RIGHT:
				blink_right();
				break;
				case HAZARD:
				hazards();
				break;
				default:
				break;
			}
			break;
			default:
			break;
		}
	}
}

void handle_can()
 {
	 if(can_read_message_if_new(&rxFrame))
	 {	
		rgbled_toggle(LED_BLUE);
		if (rxFrame.id == BRAKE_CAN_ID)
		 {
			 if (LIGHT_STATE != OFF)
			 {
				 if (rxFrame.data.u8[0])
				 {
					 rgbled_toggle(LED_RED);
					 LIGHT_STATE = BRAKE;
				 } 
				 else if (!(rxFrame.data.u8[0]) && (LIGHT_STATE == BRAKE))
				 {
					 rgbled_turn_off(LED_RED);
					 LIGHT_STATE = NORMAL;
				 }
			 }
		 }
		 if (rxFrame.id == DASHBOARD_CAN_ID)
		 {
			 if (rxFrame.data.u8[0] & LIGHTS_ON_BIT && LIGHT_STATE == OFF)
			 {
				rgbled_turn_off(LED_RED);
				rgbled_turn_on(LED_GREEN);
				LIGHT_STATE = NORMAL;
			 } 
			 else if (!(rxFrame.data.u8[0] & LIGHTS_ON_BIT) && LIGHT_STATE != OFF)
			 {
				rgbled_turn_off(LED_GREEN);
				rgbled_turn_on(LED_RED);
				LIGHT_STATE = OFF;		
			 }
			 
			 if (rxFrame.data.u8[1] & HAZARD_BIT)
			 {
				 pwm_set_duty_cycle(BR,0x00);
				 pwm_set_duty_cycle(BL,0x00);
				 BLINK_STATE = HAZARD;
			 }
			 else if (!(rxFrame.data.u8[1] & HAZARD_BIT) && (BLINK_STATE == HAZARD))
			 {
				 BLINK_STATE = NONE;
			 }
			 
			 light_level = rxFrame.data.u8[1];
		 }
		 
		 if (rxFrame.id == STEERING_WHEEL_CAN_ID)
		 {
			if (rxFrame.data.u8[1] & LEFT_BLINKER_BIT) 
			{
				rgbled_turn_on(LED_RED);
				if ((BLINK_STATE == NONE) | (BLINK_STATE == LEFT))
				{
					BLINK_STATE = LEFT;
					blink_decrement = BLINK_COUNT;
				} 
				else if (BLINK_STATE == RIGHT)
				{
					pwm_set_duty_cycle(BR,0x00);
					BLINK_STATE = LEFT;
					blink_decrement = BLINK_COUNT;
				}
			} 
			else if (rxFrame.data.u8[1] & RIGHT_BLINKER_BIT)
			{
				if ((BLINK_STATE == NONE) | (BLINK_STATE == LEFT))
				{
					BLINK_STATE = RIGHT;
					blink_decrement = BLINK_COUNT;
				} 
				else if (BLINK_STATE == RIGHT)
				{
					pwm_set_duty_cycle(BL,0x00);
					BLINK_STATE = RIGHT;
					blink_decrement = BLINK_COUNT;
				}
			}
		}
	}
 }
 
 void blink_left()
 {
	 if (blink_decrement)
	 {
		 if (!timer_is_running(TIMER1))
		 {
			 pwm_set_duty_cycle(BL,0xFF);
			 timer_start(TIMER1);
		 }
		 else if (timer_elapsed_ms(TIMER1) > 250)
		 {
			 pwm_set_duty_cycle(BL,0x00);
		 }
		 else if (timer_elapsed_ms(TIMER1) > 500)
		 {
			 pwm_set_duty_cycle(BL,0xFF);
			 timer_stop(TIMER1);
		 }
		 blink_decrement--;
	 }
	 else
	 {
		 rgbled_turn_off(LED_RED);
		 BLINK_STATE = NONE;
		 pwm_set_duty_cycle(BL,0x00);
	 }
 }
 
 void blink_right()
 {
	 if (blink_decrement)
	 {
		 if (!timer_is_running(TIMER1))
		 {
			 pwm_set_duty_cycle(BR,0xFF);
			 timer_start(TIMER1);
		 }
		 else if (timer_elapsed_ms(TIMER1) > blink_period)
		 {
			 pwm_set_duty_cycle(BR,0x00);
		 }
		 else if (timer_elapsed_ms(TIMER1) > (2*blink_period))
		 {
			 pwm_set_duty_cycle(BR,0xFF);
			 timer_stop(TIMER1);
		 }
		 blink_decrement--;
	 }
	 else
	 {
		 BLINK_STATE = NONE;
		 pwm_set_duty_cycle(BR,0x00);
	 }
 }
 
 void hazards()
 {
	 if (!timer_is_running(TIMER1))
	 {
		 pwm_set_duty_cycle(BR,0xFF);
		 pwm_set_duty_cycle(BL,0xFF);
		 timer_start(TIMER1);
	 }
	 else if (timer_elapsed_ms(TIMER1) > blink_period)
	 {
		 pwm_set_duty_cycle(BR,0x00);
		 pwm_set_duty_cycle(BL,0x00);
	 }
	 else if (timer_elapsed_ms(TIMER1) > (2*blink_period))
	 {
		 pwm_set_duty_cycle(BR,0xFF);
		 pwm_set_duty_cycle(BL,0xFF);
		 timer_stop(TIMER1);
	 }
 }