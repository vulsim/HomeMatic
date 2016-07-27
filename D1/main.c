/*
    HomeMatic D1/1 V1.1.0 (hw:v1.0) - Copyright (C) 2016 vulsim. All rights reserved	
*/

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "hardware.h"

#define MAX_INT					0xFFFF
#define SYNC_LOSS_DURATION		(MAX_INT - 938) // 1.5 of sync cycle (~15ms)
#define LONG_BLINK_DURATION		50				// 0.5 sec
#define MIN_KEY_PRESS_THRESHOLD	5				// 50 ms
#define SLIDE_START_THRESHOLD 	50				// 0.5 sec
#define STEP_THRESHOLD			5				// 100% -> 5 sec

/* INFO: Calculated for clk/256 timer and 10 ms duration (100 Hz) */

const uint16_t dim_table[100] = {
	0xFD96, 0xFDA1, 0xFDA9, 0xFDB0, 0xFDB7, 0xFDBD, 0xFDC3, 0xFDC8, 0xFDCD, 0xFDD2,
	0xFDD7, 0xFDDC, 0xFDE1, 0xFDE5, 0xFDEA, 0xFDEE, 0xFDF2, 0xFDF7, 0xFDFB, 0xFDFF,
	0xFE03, 0xFE07, 0xFE0B, 0xFE0F, 0xFE13, 0xFE17, 0xFE1B, 0xFE1E, 0xFE22, 0xFE26,
	0xFE2A, 0xFE2E, 0xFE31, 0xFE35, 0xFE39, 0xFE3C, 0xFE40, 0xFE44, 0xFE48, 0xFE4B,
	0xFE4F, 0xFE53, 0xFE56, 0xFE5A, 0xFE5E, 0xFE61, 0xFE65, 0xFE69, 0xFE6C, 0xFE70,
	0xFE74, 0xFE78, 0xFE7B, 0xFE7F, 0xFE83, 0xFE87, 0xFE8A, 0xFE8E, 0xFE92, 0xFE96,
	0xFE9A, 0xFE9E, 0xFEA2, 0xFEA6, 0xFEAA, 0xFEAE, 0xFEB2, 0xFEB7, 0xFEBB, 0xFEBF,
	0xFEC4, 0xFEC8, 0xFECC, 0xFED1, 0xFED6, 0xFEDA, 0xFEDF, 0xFEE4, 0xFEE9, 0xFEEE,
	0xFEF4, 0xFEF9, 0xFEFE, 0xFF04, 0xFF0A, 0xFF10, 0xFF16, 0xFF1D, 0xFF24, 0xFF2B,
	0xFF33, 0xFF3B, 0xFF43, 0xFF4D, 0xFF57, 0xFF62, 0xFF6F, 0xFF7F, 0xFF93, 0xFFB4
};

typedef enum {
	STATE_OFF = 0,
	STATE_SEMI_ON = 1,
	STATE_ON = 2,
	STATE_UP = 3,
	STATE_DOWN = 4
} state_t;

uint8_t blink_on;
uint8_t step_counter;
uint16_t blink_counter;
uint16_t key_press_counter;

uint8_t sync_wait;
uint8_t sync_state;
state_t state;

/* Interrupts handlers */

ISR(INT0_vect) {	
	dim_off();
	timer1_stop();

	sync_wait = 0;
	sync_state = 0;

	if (state >= STATE_SEMI_ON) {		
		timer1_set_counter(dim_table[settings.dim_level]);
		timer1_start();		
	}
}

ISR(TIMER1_OVF_vect) {	
	timer1_stop();
	
	switch (sync_state) {
		case 0:
			sync_state = 1;
			dim_on();
			timer1_set_counter(0xFF - 47);
			timer1_start();
			break;
		
		case 1:
			sync_state = 2;
			dim_off();
			timer1_set_counter(SYNC_LOSS_DURATION);
			timer1_start();
			break;
			
		case 2:
			sync_wait = 1;
			break;
	}
}

/* 1 cycle ~= 10 ms */

ISR(TIMER0_OVF_vect) {

	timer0_set_counter(255 - 156);
		
	if (is_key_pressed() && key_press_counter < MAX_INT) {
		key_press_counter++;
	}

	if (blink_counter < LONG_BLINK_DURATION) {
		blink_counter++;
	} else {
		blink_counter = 0;
		blink_on = (blink_on) ? 0 : 1;
	}
	
	if ((state == STATE_UP || state == STATE_DOWN) && step_counter <STEP_THRESHOLD) {
		step_counter++;	
	}
}

/* FSM */

void process_state(uint8_t key_pressed, uint16_t key_press_counter) {
	switch (state) {
		case STATE_OFF: {
			if (key_pressed && key_press_counter > MIN_KEY_PRESS_THRESHOLD) {
				state = STATE_SEMI_ON;
			}
			break;
		}
		
		case STATE_SEMI_ON: {
			if (!key_pressed) {
				state = STATE_ON;
			}
			break;
		}

		case STATE_ON: {
			if (key_pressed) {
				if (key_press_counter > SLIDE_START_THRESHOLD) {
					step_counter = 0;

					if (settings.dim_level > (settings.dim_level_min + settings.dim_level_max) / 2) {
						state =  STATE_DOWN;
					} else {
						state = STATE_UP;
					}					
				}
			} else if (key_press_counter > MIN_KEY_PRESS_THRESHOLD && key_press_counter < SLIDE_START_THRESHOLD) {
				state = STATE_OFF;
				
				settings_t ee_settings;
				read_settings(&ee_settings); 
				
				if (settings.dim_level != ee_settings.dim_level) {
					write_settings();
					rled_on();
					gled_off();
					_delay_ms(500);              
				}
			}
			break;
		}

		case STATE_UP: {
			if (key_pressed) {
				if (step_counter == STEP_THRESHOLD && settings.dim_level < settings.dim_level_max) {
					settings.dim_level += 1;
					step_counter = 0;
				} 
			} else {
				state = STATE_ON;
			}
			break;
		}

		case STATE_DOWN: {
			if (key_pressed) {
				if (step_counter == STEP_THRESHOLD && settings.dim_level > settings.dim_level_min) {
					settings.dim_level -= 1;
					step_counter = 0;
				}
			} else {
				state = STATE_ON;
			}
			break;
		}		
	}
}

/* Display state */

void display_state(void) {
	if (sync_wait) {
		if (blink_on) {
			rled_on();
			gled_on();
		} else {
			rled_off();
			gled_off();
		}
		return;
	} 

	switch (state) {
		case STATE_OFF: {
			rled_off();
			gled_off();
			break;
		}

		case STATE_SEMI_ON:
		case STATE_ON: {
			rled_off();
			gled_on();
			break;
		}

		case STATE_UP:			
		case STATE_DOWN: {
			rled_on();
			gled_on();
			break;
		}		
	}
}

/* Main */

int main(void) {

	cli();

	if (hardware_setup()) {
		read_settings(NULL);
		
		blink_on = 0;
		blink_counter = 0;
		key_press_counter = 0;
		sync_wait = 1;
		sync_state = 0;
		state = STATE_OFF;

		sei();
		timer0_set_counter(255 - 156);
		timer0_start();
		
		for (;;) {
			uint8_t key_pressed = is_key_pressed();

			process_state(key_pressed, key_press_counter);
			display_state();

			if (!key_pressed) {
				timer0_disable_isr();
				key_press_counter = 0;
				timer0_enable_isr();
			}
		}
	}

	rled_on();
	gled_off();

	return 0;
}
